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

int __t_spawn(struct t_info *state);				// spawn a new thread
int __t_exit(int status);							// exit the current thread
int __t_kill(int thread, int status);				// kill another thread
int __t_reap(int thread, struct t_info *info);		// reap a zombie thread
int __t_wait(int event, int *status);				// wait for an event
int __t_getid(void);								// get the current thread ID
int __t_yield(void);								// yield thread timeslice
int __t_getdead(void);                              // get next dead thread ID
int __t_getfault(void);                             // get next faulted thread ID
int __t_pause(int thread);							// pause thread execution
int __t_resume(int thread);							// resume a paused thread
int __t_getstate(int thread, struct t_info *info);	// examine a paused thread
int __t_setstate(int thread, struct t_info *info);	// modify a paused thread
int __t_sysret(uint32_t regs[6]);                   // switch to user mode

#define REG_EAX 0
#define REG_EBX 1
#define REG_ECX 2
#define REG_EDX 3
#define REG_EDI 4
#define REG_ESI 5

int __irq_wait(int irq);							// wait for an IRQ to fire
int __irq_reset(int irq);							// reset an IRQ

/* paging contexts **********************************************************/

int pctx_new(void);
int pctx_free(int pctx);
int t_set_pctx(int thread, int pctx);
int t_get_pctx(int thread);

/* paging *******************************************************************/

int      p_alloc    (uint32_t page, int flags);
int      p_set_frame(uint32_t page, uint64_t frame);
int      p_set_flags(uint32_t page, int flags);
uint64_t p_get_frame(uint32_t page);
int      p_get_flags(uint32_t page);

uint64_t newframe(void);
int freeframe(uint64_t frame);

#endif/*__SYSTEM_KERNEL_H*/
