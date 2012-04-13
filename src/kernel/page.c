/*
 * Copyright (C) 2009-2012 Nick Johnson <nickbjohnson4224 at gmail.com>
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

#include "rho/arch.h"

#include "string.h"
#include "space.h"
#include "cpu.h"

frame_t *cmap = (void*) 0xFFFFF000; /* Current page directory mapping */
frame_t *ctbl = (void*) 0xFFC00000; /* Current base page table mapping */

/****************************************************************************
 * page_get
 *
 * Returns the value of a page in the current address space.
 */

frame_t page_get(uintptr_t page) {

	if ((cmap[page >> 22] & PF_PRES) == 0) {
		return 0;
	}

	return ctbl[page >> 12];
}

/****************************************************************************
 * page_set
 *
 * Sets a page in the current address space to a value.
 */

void page_set(uintptr_t page, frame_t value) {

	if ((cmap[page >> 22] & PF_PRES) == 0) {
		page_touch(page);
	}

	ctbl[page >> 12] = value;
	cpu_flush_tlb_part(page);
}

/****************************************************************************
 * page_touch
 *
 * Ensures that the segment containing a page exists.
 */

void page_touch(uintptr_t page) {
	page &= ~0x3FFFFF;

	if (cmap[page >> 22] & PF_PRES) {
		return;
	}

	cmap[page >> 22]  = frame_new() | PF_PRES | PF_RW | PF_USER;

	cpu_flush_tlb_part((uintptr_t) &ctbl[page >> 12]);

	memclr(&ctbl[page >> 12], 0x1000);
}
