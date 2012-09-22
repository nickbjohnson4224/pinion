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

#include "interrupt.h"
#include "string.h"
#include "thread.h"
#include "space.h"
#include "debug.h"
#include "pctx.h"
#include "cpu.h"

static struct thread *_active_thread;
static struct thread *_thread_table[THREAD_COUNT];

/*****************************************************************************
 * thread_alloc
 *
 * Allocate a new thread structure. The state of this new thread is TS_PAUSED.
 * If successful, returns a pointer to the new thread structure. If not
 * successful, returns NULL.
 */

static struct thread *thread_alloc(void) {
	struct thread *thread;

	thread = heap_alloc(sizeof(struct thread));
	thread->fxdata = heap_alloc(512);

	memclr(thread, sizeof(struct thread));

	int i;
	for (i = 0; i < THREAD_COUNT; i++) {
		if (!_thread_table[i]) break;
	}
	if (i >= THREAD_COUNT) return NULL;

	_thread_table[i] = thread;
	thread->id = i;
	thread->state = TS_PAUSED;

	return thread;
}

/*****************************************************************************
 * thread_get
 *
 * Return the thread structure associated with the given thread ID. If no
 * thread is associated with the given ID (i.e. if that ID is not allocated
 * or is out of range), returns NULL.
 */

struct thread *thread_get(int thread) {
	if (thread < 0 || thread >= THREAD_COUNT) return NULL;
	return _thread_table[thread];
}

/*****************************************************************************
 * thread_kill
 *
 * Free all memory associated with a given thread structure. The state of the
 * thread changes to TS_FREE.
 */

void thread_kill(struct thread *thread) {

	_thread_table[thread->id] = NULL;

	/* free FPU/SSE data */
	if (thread->fxdata) {
		heap_free(thread->fxdata, 512);
		thread->fxdata = NULL;
	}

	/* free thread structure */
	heap_free(thread, sizeof(struct thread));
}

/*****************************************************************************
 * thread_get_active
 * 
 * Return the actively running thread. If no thread is running, returns NULL.
 * The actively running thread is guaranteed to be the unique thread with
 * state TS_RUNNING.
 */

struct thread *thread_get_active(void) {
	return _active_thread;
}

/*****************************************************************************
 * thread_load
 *
 * Load the state of a given thread structure, i.e. prepare the processor for
 * running the given thread. If the active paging context does not match the
 * paging context of the given thread, it is changed to match the paging
 * context of the given thread.
 */

int thread_load(struct thread *thread) {
	
	if (thread->vm86_active) {
		set_int_stack(&thread->vm86_start);
	}
	else {
		set_int_stack(&thread->vm86_es);
	}

	if (thread->fxdata) {
		fpu_load(thread->fxdata);
	}

	if (thread->pctx && thread->pctx != _active_pctx) {
		pctx_load(thread->pctx);
	}

	_active_thread = thread;

	return 0;
}

/*****************************************************************************
 * thread_save
 *
 * Fully save the state of a given thread structure. This is only required
 * before switching to another thread.
 */

int thread_save(struct thread *thread) {
	
	if (thread && thread->fxdata) {
		fpu_save(thread->fxdata);
	}

	_active_thread = NULL;

	return 0;
}

/*****************************************************************************
 * thread_new
 *
 * Allocate a new thread and return its ID number. Returns -1 on error. The
 * thread's state is TS_PAUSED.
 */

int thread_new(void) {
	struct thread *thread = thread_alloc();

	if (!thread) {
		debug_printf("error: thread table full\n");
		return -1;
	}

	return thread->id;
}

/* scheduler ****************************************************************/

struct thread *sched_head;
struct thread *sched_tail;

int schedule_push(struct thread *thread) {

	if (!sched_tail) {
		sched_head = thread;
	}
	else {
		sched_tail->next = thread;
	}
	sched_tail = thread;
	thread->next = NULL;

	return 0;
}

int schedule_remv(struct thread *thread) {
	
	if (!sched_head) {
		return 1;
	}

	if (thread == sched_head) {
		sched_head = sched_head->next;
		if (!sched_head) {
			sched_tail = NULL;
		}
	}
	else {
		for (struct thread *t1 = sched_head; t1->next; t1 = t1->next) {
			if (t1->next == thread) {
				t1->next = thread->next;
				if (sched_tail == thread) {
					sched_tail = t1;
				}
				return 0;
			}
		}

		return 1;
	}

	return 0;
}

struct thread *schedule_next(void) {
	return sched_head;
}

/* event queues *************************************************************/

static struct evqueue {
	struct event *ev_head;
	struct event *ev_tail;

	struct thread *front_thread;
	struct thread *back_thread;
} evqueue[EV_COUNT];

int irqstate[EV_COUNT];

int event_wait(int id, int event) {
	struct thread *thread = thread_get(id);

	if (!thread) {
		return 1;
	}

	if (event < 0) {
		if (irqstate[~event]) {
			return 1;
		}
		return 0;
	}	

	if (event < 0 || event >= EV_COUNT) {
		return 1;
	}

	if (irqstate[event]) {
		thread_save(thread);
		schedule_push(thread);
		thread->state = TS_QUEUED;
	}
	else {
		/* queue thread */
		if (!evqueue[event].back_thread) {
			evqueue[event].front_thread = thread;
		}
		else {
			evqueue[event].back_thread->next_evqueue = thread;
		}
		evqueue[event].back_thread = thread;
		thread->next_evqueue = NULL;
		thread->event = event;

		thread_save(thread);
		thread->state = TS_WAITING;
	}

	return 0;
}

int event_remv(int id, int event) {
	struct thread *thread = thread_get(id);

	if (!thread) {
		return 1;
	}

	if (event < 0 || event >= EV_COUNT) {
		return 1;
	}

	if (!evqueue[event].front_thread) {
		return 1;
	}

	if (evqueue[event].front_thread == thread) {
		evqueue[event].front_thread = thread->next;
		if (evqueue[event].back_thread == thread) {
			evqueue[event].back_thread = NULL;
		}
		return 0;
	}
	else for (struct thread *t = evqueue[event].front_thread; t->next; t = t->next) {
		if (t->next == thread) {
			t->next = thread->next;
			if (evqueue[event].back_thread == thread) {
				evqueue[event].back_thread = t;
			}
			return 0;
		}
	}

	return 1;
}

int event_send(int id, int event) {

	if (event < 0 || event >= EV_COUNT) {
		return 1;
	}

	if (evqueue[event].front_thread) {

		// handle event now
		struct thread *thread = evqueue[event].front_thread;
		evqueue[event].front_thread = thread->next_evqueue;
		if (!thread->next_evqueue) evqueue[event].back_thread = NULL;

		thread->eax = event;
		thread->event = -1;

		if (thread->state == TS_RUNNING) {
			thread_save(thread);
		}
		schedule_push(thread);
		thread->state = TS_QUEUED;
	}

	if (event < 240) {
		irq_mask(event);
		irq_reset(event);
		irqstate[event] = 1;
	}

	return 0;
}

/* dead queue ***************************************************************/

static struct thread *dead_tail;
static struct thread *dead_head;

static struct thread *reaper_tail;
static struct thread *reaper_head;

int dead_push(struct thread *thread) {

	if (reaper_tail) {

		// retrieve next reaper
		struct thread *reaper;
		reaper = reaper_tail;
		reaper_tail = reaper_tail->next_dead;
		if (!reaper_tail) {
			reaper_head = NULL;
		}

		// notify reaper
		reaper->eax = thread->id;
		schedule_push(reaper);
		reaper->state = TS_QUEUED;

		return 0;
	}

	if (dead_head) {
		dead_head->next_dead = thread;
		thread->next_dead = NULL;
		dead_head = thread;
	}
	else {
		dead_head = thread;
		dead_tail = thread;
	}

	return 0;
}

struct thread *dead_peek(void) {
	return dead_tail;
}

struct thread *dead_pull(void) {
	struct thread *dead;

	if (!dead_tail) {
		return NULL;
	}

	dead = dead_tail;
	dead_tail = dead_tail->next_dead;
	
	if (!dead_tail) {
		dead_head = NULL;
	}

	return dead;
}

int dead_wait(struct thread *reaper) {

	if (dead_tail) {

		// retrieve next dead
		struct thread *dead = dead_pull();

		// notify reaper
		reaper->eax = dead->id;
		schedule_push(reaper);
		reaper->state = TS_QUEUED;

		return 0;
	}

	if (reaper_head) {
		reaper_head->next_dead = reaper;
		reaper->next_dead = NULL;
		reaper_head = reaper;
	}
	else {
		reaper_head = reaper;
		reaper_tail = reaper;
	}

	return 0;
}

/* fault queue **************************************************************/

static struct thread *fault_head;
static struct thread *fault_tail;

static struct thread *debug_head;
static struct thread *debug_tail;

int fault_push(struct thread *fault) {

	if (debug_tail) {

		// retrieve next debugger
		struct thread *debug;
		debug = debug_tail;
		debug_tail = debug_tail->next_fault;
		if (!debug_tail) {
			debug_head = NULL;
		}

		// notify debugger
		debug->eax = fault->id;
		schedule_push(debug);
		debug->state = TS_QUEUED;

		return 0;
	}

	if (fault_head) {
		fault_head->next_fault = fault;
		fault->next_fault = NULL;
		fault_head = fault;
	}
	else {
		fault_head = fault;
		fault_tail = fault;
	}

	return 0;
}

struct thread *fault_peek(void) {
	return fault_tail;
}

struct thread *fault_pull(void) {
	struct thread *fault;

	if (!fault_tail) {
		return NULL;
	}

	fault = fault_tail;	
	fault_tail = fault_tail->next;

	if (!fault_tail) {
		fault_head = NULL;
	}

	return fault;
}

int fault_wait(struct thread *debug) {

	if (fault_tail) {

		// retrieve next faulting thread
		struct thread *fault = fault_pull();

		// notify debugger
		debug->eax = fault->id;
		schedule_push(debug);
		debug->state = TS_QUEUED;
		
		return 0;
	}

	if (debug_head) {
		debug_head->next_fault = debug;
		debug->next_fault = NULL;
		debug_head = debug;
	}
	else {
		debug_head = debug;
		debug_tail = debug;
	}

	return 0;
}
