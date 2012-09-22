/*
 * Copyright (C) 2012 Nick Johnson <nickbjohnson4224 at gmail.com>
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <config/address.h>
#include "arch.h"

#include <pinion.h>

#include "string.h"
#include "space.h"
#include "debug.h"
#include "pctx.h"
#include "cpu.h"

static void space_exmap(space_t space);
static space_t space_alloc(void);
static space_t space_clone(void);
static void space_free(space_t space);

static space_t _pctx_table[PCTX_COUNT];
int _active_pctx;

static void _pctx_init(void) {
	_pctx_table[0] = space_clone();
}

int pctx_new(void) {
	
	if (!_pctx_table[0]) {
		_pctx_init();
	}

	for (int i = 0; i < PCTX_COUNT; i++) {
		if (!_pctx_table[i]) {
			_pctx_table[i] = space_clone();
			return i;
		}
	}

	return -1;
}

int pctx_free(int pctx) {

	if (pctx <= 0 || pctx >= PCTX_COUNT) return 1;
	if (!_pctx_table[pctx]) return 1;

	space_free(_pctx_table[pctx]);
	_pctx_table[pctx] = 0;

	return 0;
}

int pctx_load(int pctx) {

	if (pctx <= 0 || pctx >= PCTX_COUNT) return 1;
	if (!_pctx_table[pctx]) return 1;

	cpu_set_cr3(_pctx_table[pctx]);
	_active_pctx = pctx;

	return 0;
}

/****************************************************************************
 * space_alloc
 *
 * Returns a new address space.
 */

static space_t space_alloc(void) {
	space_t space = frame_new();
	uint32_t *map = (void*) TMP_SRC;

	/* clear new space */
	page_set(TMP_SRC, page_fmt(space, PF_PRES | PF_RW));
	memclr(map, PAGESZ);

	/* set recursive mapping */
	map[PGE_MAP >> 22] = page_fmt(space, PF_PRES | PF_RW);

	return space;
}

/****************************************************************************
 * space_clone
 *
 * Copies the currently loaded address space so that the new address space 
 * is independent of the old one.
 */

static space_t space_clone() {
	uint32_t i;
	space_t dest;
	frame_t *exmap;

	/* Create new address space */
	dest = space_alloc();

	/* Exmap in new address space */
	space_exmap(dest);
	exmap = (void*) (TMP_MAP + 0x3FF000);

	/* Clone/clear userspace */
	for (i = SYSTEM_ADDR_BASE >> 22; i < 1023; i++) {
		exmap[i] = cmap[i];
	}

	cpu_flush_tlb_full();

	return dest;
}

/****************************************************************************
 * space_exmap
 *
 * Recursively maps an external address space.
 */

static void space_exmap(space_t space) {
	cmap[TMP_MAP >> 22] = page_fmt(space, PF_PRES | PF_RW);
	cpu_flush_tlb_full();
}

/****************************************************************************
 * space_free
 *
 * Completely frees resources associated with an address space.
 */

static void space_free(space_t space) {
	uint32_t i, j;
	frame_t *extbl, *exmap;

	space_exmap(space);
	extbl = (void*) TMP_MAP;
	exmap = (void*) (TMP_MAP + 0x3FF000);

	for (i = 0; i < SYSTEM_ADDR_BASE >> 22; i++) {
		if (exmap[i] & PF_PRES) {
			for (j = 0; j < 1024; j++) {
				if (extbl[i * 1024 + j] & PF_PRES) {
					frame_free(page_ufmt(extbl[i * 1024 + j]));
				}
				extbl[i * 1024 + j] = 0;
			}
			frame_free(page_ufmt(exmap[i]));
			exmap[i] = 0;
		}
	}

	frame_free(space);
}
