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

#ifndef __SYSTEM_KERNEL_H
#define __SYSTEM_KERNEL_H

#include <stdint.h>
#include <pinion.h>

/* kernel call *************************************************************/

int kcall(int call, int arg0, int arg1, int arg2, int arg3);

/* specific calls **********************************************************/

int spawn(void *stack, void (*entry)(void));
int exit(int status);
int kill(int thread, int status);
int reap(int thread, struct t_info *info);
int wait(int event, int *status);
int gettid(void);
int yield(void);
int wakeup(int thread, int event, int status);
int renice(int thread, int status);
int pause(int thread);
int resume(int thread);
int t_info(int thread, struct t_info *info);
int fixup(int thread, struct t_info *info);
int setcall(void (*entry)(void));

int newpctx(void);
int freepctx(int pctx);
int setpctx(int thread, int pctx);
int getpctx(int thread);
int alloc(uint32_t page, int flags);
int setframe(uint32_t page, uint64_t frame);
int setflags(uint32_t page, int flags);
uint64_t getframe(uint32_t page);
int getflags(uint32_t page);

#endif/*__SYSTEM_KERNEL_H*/
