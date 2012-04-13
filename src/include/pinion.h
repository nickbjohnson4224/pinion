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

#ifndef __PINION_ABI_H
#define __PINION_ABI_H

#include <stdint.h>

/* thread information structure *********************************************/

struct t_regs {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t eip;
	uint32_t eflags;

	uint32_t fxdata[128];
} __attribute__((packed));

struct t_info {
	int id;
	int state;
	int exit_status;
	int wait_event;

	uint64_t tick;
	
	struct t_regs regs;

	uint32_t fault_addr;
	uint32_t fault_type;
	uint32_t fault_code;
} __attribute__((packed));

/* thread states ************************************************************/

#define THREAD_COUNT 1024

#define TS_FREE    0 // not currently an allocated thread
#define TS_QUEUED  1 // in a scheduler queue
#define TS_RUNNING 2 // currently running on a processor
#define TS_WAITING 3 // waiting for an event
#define TS_PAUSED  4 // inactive due to pause() or a fault
#define TS_ZOMBIE  5 // killed but not yet reaped

/* threading calls **********************************************************/

#define KCALL_SPAWN    0x00 // int spawn(void *stack, void *entry)
#define KCALL_EXIT     0x01 // int exit(int status)
#define KCALL_KILL     0x02 // int kill(int thread, int status)
#define KCALL_REAP     0x03 // int reap(int thread, struct t_info *info)
#define KCALL_WAIT     0x04 // int wait(int event, int *status)
#define KCALL_GETTID   0x05 // int gettid(void)
#define KCALL_YIELD    0x06 // int yield(void)
#define KCALL_WAKEUP   0x07 // int wakeup(int thread, int event, int status)
#define KCALL_RENICE   0x08 // int renice(int priority)
#define KCALL_PAUSE    0x09 // int pause(int thread)
#define KCALL_RESUME   0x0A // int resume(int thread)
#define KCALL_INFO     0x0B // int info(int thread, struct t_info *info)
#define KCALL_FIXUP    0x0C // int fixup(int thread, struct t_info *info)
#define KCALL_SETCALL  0x0D // int setcall(void *entry)

/* event constants and macros ***********************************************/

#define EV_COUNT  1024        // number of valid event vectors
#define EV_EXIT   0           // event caused by thread exit
#define EV_FAULT  1           // event caused by thread fault (non-page)
#define EV_PAGE   2           // event caused by thread page fault
#define EV_IRQ(n) (512 + (n)) // 

/* memory management calls **************************************************/

#define KCALL_NEWPCTX  0x10 // int newpctx(void)
#define KCALL_FREEPCTX 0x11 // int freepctx(int pctx)
#define KCALL_SETPCTX  0x12 // int setpctx(int thread, int pctx)
#define KCALL_GETPCTX  0x13 // int getpctx(int thread)
#define KCALL_ALLOC    0x14 // int alloc(uintptr_t page, int flags)
#define KCALL_SETFRAME 0x15 // int setframe(uintptr_t page, uint64_t frame)
#define KCALL_SETFLAGS 0x16 // int setflags(uintptr_t page, int flags)
#define KCALL_GETFRAME 0x17 // uint64_t getframe(uintptr_t page)
#define KCALL_GETFLAGS 0x18 // int getflags(uintptr_t page)

/* memory management constants and macros ***********************************/

#define PCTX_COUNT 1024

#define PFLAG_PRES  0x001
#define PFLAG_WRITE 0x002
#define PFLAG_USER	0x004
#define PFLAG_EXEC  0x000

#endif/*__PINION_ABI_H*/
