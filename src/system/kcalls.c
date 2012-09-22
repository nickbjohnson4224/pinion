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

#include <pinion.h>
#include "kernel.h"

extern int kcall(int call, int arg0, int arg1, int arg2, int arg3);

int __t_spawn(struct t_info *state) {
	return kcall(KCALL_SPAWN, (int) state, 0, 0, 0);
}

int __t_exit(int status) {
	struct t_info state;
	int err;

	err = __t_getstate(-1, &state);
	if (err) return err;

	state.flags |= TF_DEAD;
	state.regs.eax = status;

	err = __t_setstate(-1, &state);
	if (err) return err;

	return 0;
}

int __t_kill(int thread, int status) {
	struct t_info state;
	int err;

	err = __t_pause(thread);
	if (err) return err;

	err = __t_getstate(thread, &state);
	if (err) return err;

	state.flags |= TF_DEAD;
	state.regs.eax = status;

	err = __t_setstate(thread, &state);
	if (err) return err;

	return 0;
}

int __t_reap(int thread, struct t_info *info) {
	return kcall(KCALL_REAP, thread, (int) info, 0, 0);
}

int __t_getid(void) {
	return kcall(KCALL_GETTID, 0, 0, 0, 0);
}

int __t_yield(void) {
	return kcall(KCALL_YIELD, 0, 0, 0, 0);
}

int __t_pause(int thread) {
	return kcall(KCALL_PAUSE, thread, 0, 0, 0);
}

int __t_resume(int thread) {
	return kcall(KCALL_RESUME, thread, 0, 0, 0);
}

int __t_getstate(int thread, struct t_info *state) {
	return kcall(KCALL_GETSTATE, thread, (int) state, 0, 0);
}

int __t_setstate(int thread, struct t_info *state) {
	return kcall(KCALL_SETSTATE, thread, (int) state, 0, 0);
}

int __t_getdead(void) {
	return kcall(KCALL_GETDEAD, 0, 0, 0, 0);
}

int __t_getfault(void) {
	return kcall(KCALL_GETFAULT, 0, 0, 0, 0);
}

int __irq_wait(int irq) {
	return kcall(KCALL_WAIT, irq, 0, 0, 0);
}

int __irq_reset(int irq) {
	return kcall(KCALL_RESET, irq, 0, 0, 0);
}

int pctx_new(void) {
	return kcall(KCALL_NEWPCTX, 0, 0, 0, 0);
}

int pctx_free(int pctx) {
	return kcall(KCALL_FREEPCTX, pctx, 0, 0, 0);
}

uint64_t newframe(void) {
	return kcall(KCALL_NEWFRAME, 0, 0, 0, 0);
}

int freeframe(uint64_t frame) {
	return kcall(KCALL_FREEFRAME, frame & 0xFFFFFFFF, frame >> 32ULL, 0, 0);
}

int takeframe(uint64_t frame) {
	return kcall(KCALL_TAKEFRAME, frame & 0xFFFFFFFF, frame >> 32ULL, 0, 0);
}

int p_alloc(uint32_t page, int flags) {
	uint64_t frame = newframe();

	if (frame == 0xFFFFFFFFFFFFFFFFULL) return 1;

	p_set_frame(page, frame);
	p_set_flags(page, flags);

	return 0;
}

int p_set_frame(uint32_t page, uint64_t frame) {
	return kcall(KCALL_SETFRAME, page, frame, 0, 0);
}

int p_set_flags(uint32_t page, int flags) {
	return kcall(KCALL_SETFLAGS, page, flags, 0, 0);
}

uint64_t p_get_frame(uint32_t page) {
	return kcall(KCALL_GETFRAME, page, 0, 0, 0);
}

int p_get_flags(uint32_t page) {
	return kcall(KCALL_GETFLAGS, page, 0, 0, 0);
}
