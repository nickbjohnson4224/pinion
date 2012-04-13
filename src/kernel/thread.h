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

#ifndef KERNEL_THREAD_H
#define KERNEL_THREAD_H

#include <stdint.h>
#include <pinion.h>

#include "types.h"

/* thread structure ********************************************************/

struct event {
	struct event *next;
	int status;
	int event;
};

struct thread {

	/* stored continuation */
	uint32_t ds;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t num;
	uint32_t err;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t useresp;
	uint32_t ss;

	uint32_t vm86_es;
	uint32_t vm86_ds;
	uint32_t vm86_fs;
	uint32_t vm86_gs;
	uint32_t vm86_start;
	uint32_t vm86_active;
	uint32_t vm86_if;
	uint32_t vm86_saved_eip;
	uint32_t vm86_saved_esp;
	uint32_t vm86_saved_eflags;

	uint32_t *fxdata;

	/* thread state */
	int id;
	int state;
	int exit_status;
	uint32_t pfault_addr;

	/* scheduler information */
	uint64_t tick;
	struct thread *next;

	/* event queue information */
	struct thread *next_evqueue;
	struct event *ev_head;
	struct event *ev_tail;
	int event;

	/* paging context */
	int pctx;

} __attribute__ ((packed));

/* thread operations *******************************************************/

struct thread *thread_get_active(void);
struct thread *thread_get (int thread);
void           thread_kill(struct thread *image);

int thread_new(void);
int thread_get_state(int thread);
int thread_set_state(int thread, int state);

/* scheduler ****************************************************************/

struct thread *schedule_next(void);

/* event queue **************************************************************/

int event_wait(int thread, int event);
int event_remv(int thread, int event);
int event_send(int thread, int event, int status);

#endif/*KERNEL_THREAD_H*/
