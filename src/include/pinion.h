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
	int32_t id;    // thread ID
	int32_t pctx;  // paging context ID

	uint8_t state; // thread state 
	uint8_t flags; // thread flags
	uint8_t event; // event being waited on
	uint8_t fault; // fault that occurred

	// fault information
	uint32_t fault_addr;
	uint8_t fault_reserved[4];

	// scheduling information
	int8_t sched_priority;
	int8_t sched_flags;
	uint8_t sched_reserved[2];
	uint32_t sched_ticks;

	// system call state
	uint32_t usr_ip; // user saved instruction pointer
	uint32_t usr_sp; // user saved stack pointer

	struct t_regs regs;

} __attribute__((packed));

/* thread states ************************************************************/

#define THREAD_COUNT 1024

#define TS_FREE    0 // not currently an allocated thread
#define TS_QUEUED  1 // in a scheduler queue
#define TS_RUNNING 2 // currently running on a processor
#define TS_WAITING 3 // waiting for an event
#define TS_PAUSED  4 // paused due to pause() or a fault
#define TS_PAUSEDW 5 // paused while waiting

#define TF_DEAD    1 // thread has exited or been killed
#define TF_USER    2 // thread is in usermode

#define TE_STATE   1 // invalid state transition
#define TE_EXIST   2 // thread does not exist
#define TE_RESRC   3 // insufficient resources to fulfill request

/* kernel calls *************************************************************/

#define KCALL_KINFO    0x00 // int kinfo(struct k_info *info);
#define KCALL_KCONFIG  0x01 // int kconfig(struct k_info *info);

/* threading calls **********************************************************/

#define KCALL_SPAWN    0x02 // int spawn(void *stack, void *entry)
#define KCALL_GETTID   0x03 // int gettid(void)
#define KCALL_YIELD    0x04 // int yield(void)
#define KCALL_PAUSE    0x05 // int pause(int thread)
#define KCALL_RESUME   0x06 // int resume(int thread)
#define KCALL_GETSTATE 0x07 // int getstate(int thread, struct t_info *info)
#define KCALL_SETSTATE 0x08 // int setstate(int thread, struct t_info *info)
#define KCALL_GETFAULT 0x09 // int getfault(void)
#define KCALL_GETDEAD  0x0A // int getdead(void)
#define KCALL_REAP     0x0B // int reap(int thread, struct t_info *info)
#define KCALL_WAIT     0x0D // int wait(int event)
#define KCALL_RESET    0x0E // int reset(int event)
#define KCALL_SYSRET   0x0F // (used from assembly only)

#define FV_PAGE 1 // page fault
#define FV_ACCS 2 // access violation (other than page fault)

/* event constants and macros ***********************************************/

#define EV_COUNT 256 // number of valid event vectors

// EV_VTIMER(n) is fired at 2^(n-5) Hz
// n may be in the range from 0 to 15 inclusive
// i.e. there are timers from 1/32 to 1024 Hz
#define EV_VTIMER(n) (255-(n))

/* paging calls *************************************************************/

#define KCALL_NEWPCTX  0x10 // int newpctx(void)
#define KCALL_FREEPCTX 0x11 // int freepctx(int pctx)
#define KCALL_SETFRAME 0x12 // int setframe(uintptr_t page, uint64_t frame)
#define KCALL_SETFLAGS 0x13 // int setflags(uintptr_t page, int flags)
#define KCALL_GETFRAME 0x14 // uint64_t getframe(uintptr_t page)
#define KCALL_GETFLAGS 0x15 // int getflags(uintptr_t page)

#define KCALL_NEWFRAME  0x1C // uint64_t newframe(void);
#define KCALL_FREEFRAME 0x1D // int freeframe(uint64_t frame);
#define KCALL_TAKEFRAME 0x1E // int takeframe(uint64_t frame);

/* memory management constants and macros ***********************************/

#define PCTX_COUNT 1024

#define PFLAG_PRES  0x001
#define PFLAG_WRITE 0x002
#define PFLAG_USER	0x004
#define PFLAG_EXEC  0x000

#endif/*__PINION_ABI_H*/
