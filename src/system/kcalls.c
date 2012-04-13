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

#include "kernel.h"

int spawn(void *stack, void (*entry)(void)) {
	return kcall(KCALL_SPAWN, (int) stack, (int) entry, 0, 0);
}

int exit(int status) {
	return kcall(KCALL_EXIT, status, 0, 0, 0);
}

int kill(int thread, int status) {
	return kcall(KCALL_KILL, thread, status, 0, 0);
}

int wait(int event, int *status) {
	extern int _wait(int event, int *status);
	return _wait(event, status);
}

int reap(int thread, struct t_info *info) {
	return kcall(KCALL_REAP, thread, (int) info, 0, 0);
}

int gettid(void) {
	return kcall(KCALL_GETTID, 0, 0, 0, 0);
}

int yield(void) {
	return kcall(KCALL_YIELD, 0, 0, 0, 0);
}

int wakeup(int thread, int event, int status) {
	return kcall(KCALL_WAKEUP, thread, event, status, 0);
}

int renice(int thread, int priority) {
	return kcall(KCALL_RENICE, thread, priority, 0, 0);
}

int pause(int thread) {
	return kcall(KCALL_PAUSE, thread, 0, 0, 0);
}

int resume(int thread) {
	return kcall(KCALL_RESUME, thread, 0, 0, 0);
}

int t_info(int thread, struct t_info *info) {
	return kcall(KCALL_INFO, thread, (int) info, 0, 0);
}

int fixup(int thread, struct t_info *info) {
	return kcall(KCALL_FIXUP, thread, (int) info, 0, 0);
}

int newpctx(void) {
	return kcall(KCALL_NEWPCTX, 0, 0, 0, 0);
}

int freepctx(int pctx) {
	return kcall(KCALL_FREEPCTX, pctx, 0, 0, 0);
}

int setpctx(int thread, int pctx) {
	return kcall(KCALL_SETPCTX, thread, pctx, 0, 0);
}

int getpctx(int thread) {
	return kcall(KCALL_GETPCTX, thread, 0, 0, 0);
}

int alloc(uint32_t page, int flags) {
	return kcall(KCALL_ALLOC, page, flags, 0, 0);
}

int setframe(uint32_t page, uint64_t frame) {
	return kcall(KCALL_SETFRAME, page, frame, 0, 0);
}

int setflags(uint32_t page, int flags) {
	return kcall(KCALL_SETFLAGS, page, flags, 0, 0);
}

uint64_t getframe(uint32_t page) {
	return kcall(KCALL_GETFRAME, page, 0, 0, 0);
}

int getflags(uint32_t page) {
	return kcall(KCALL_GETFLAGS, page, 0, 0, 0);
}
