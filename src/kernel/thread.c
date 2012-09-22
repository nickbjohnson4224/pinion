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

static int schedule_push(struct thread *thread);
static int schedule_remv(struct thread *thread);

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

static int thread_load(struct thread *thread) {
	
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

	return 0;
}

/*****************************************************************************
 * thread_save
 *
 * Fully save the state of a given thread structure. This is only required
 * before switching to another thread.
 */

static int thread_save(struct thread *thread) {
	
	if (thread && thread->fxdata) {
		fpu_save(thread->fxdata);
	}

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

/*****************************************************************************
 * thread_get_state
 *
 * Return the current state of the thread associated with the given thread ID.
 */

int thread_get_state(int id) {
	struct thread *thread = thread_get(id);

	if (!thread) {
		return TS_FREE;
	}

	return thread->state;
}

static void invtrans(int id, int state0, int state1) {
	static const char *tsnames[6] = {
		"FREE", "QUEUED", "RUNNING", "WAITING", "PAUSED", "ZOMBIE" };

	const char *name0 = (state0 < 0 || state0 > 5) ? "INVALID" : tsnames[state0];
	const char *name1 = (state1 < 0 || state1 > 5) ? "INVALID" : tsnames[state1];

	debug_printf("error: %d: state transition %s -> %s\n", id, name0, name1);
	debug_panic("invalid thread state transition");
}

/*****************************************************************************
 * thread_set_state
 *
 * Perform a state transition for the given thread to the given state. If this
 * transition is invalid, the kernel will panic. Any code that is able to
 * request an invalid state transition is incorrect and must be fixed. This is
 * the only function that should be able to modify the state of a thread
 * (other than thread_kill and thread_alloc, which transition in and out of
 * TS_FREE and are internal anyway.)
 */

int thread_set_state(int id, int state) {
	struct thread *thread = thread_get(id);

	/* transitions out of TS_FREE are invalid */
	if (!thread) {
		invtrans(id, TS_FREE, state);
		return 1;
	}

	/* redundant state transitions are invalid */
	if (thread->state == state) {
		invtrans(id, thread->state, state);
		return 1;
	}

	switch (thread->state) {
	case TS_QUEUED:
		switch (state) {
		case TS_RUNNING:						/* schedule */
			thread->state = TS_RUNNING;
			thread_load(thread);
			_active_thread = thread;
			schedule_remv(thread);
			break;

		case TS_PAUSED:							/* pause */
			thread->state = TS_PAUSED;
			schedule_remv(thread);
			break;

		case TS_ZOMBIE:							/* kill */
			thread->state = TS_ZOMBIE;
			schedule_remv(thread);
			break;

		default:
			invtrans(id, thread->state, state);
			return 1;
		}
		break;

	case TS_RUNNING:
		switch (state) {
		case TS_QUEUED:							/* yield or preempt */
			thread->state = TS_QUEUED;
			schedule_push(thread);
			break;

		case TS_WAITING:						/* wait */
			thread->state = TS_WAITING;
			break;

		case TS_PAUSED:							/* fault or pause */
			thread->state = TS_PAUSED;
			break;

		case TS_ZOMBIE:							/* exit or kill */
			thread->state = TS_ZOMBIE;
			break;

		default:
			invtrans(id, thread->state, state);
			return 1;
		}
		thread_save(thread);
		_active_thread = NULL;
		break;

	case TS_WAITING:
		switch (state) {
		case TS_QUEUED:							/* wakeup */
			thread->state = TS_QUEUED;
			schedule_push(thread);
			break;

		case TS_ZOMBIE:							/* kill */
			thread->state = TS_ZOMBIE;
			break;

		case TS_PAUSED:							/* pause */
			thread->state = TS_PAUSED;
			event_remv(thread->id, thread->event);
			break;

		default:
			invtrans(id, thread->state, state);
			return 1;
		}
		break;

	case TS_PAUSED:
		switch (state) {
		case TS_QUEUED:							/* resume */
			thread->state = TS_QUEUED;
			schedule_push(thread);
			break;

		case TS_WAITING:
			thread->state = TS_WAITING;			/* resume */
			break;

		case TS_ZOMBIE:							/* kill */
			thread->state = TS_ZOMBIE;
			break;

		default:
			invtrans(id, thread->state, state);
			return 1;
		}
		break;

	case TS_ZOMBIE:

		if (state != TS_FREE) 
			invtrans(id, thread->state, state);
		thread_kill(thread);					/* reap */
		break;

	default:
		invtrans(id, thread->state, state);
		return 1;
	}

	return 0;
}

/* scheduler ****************************************************************/

struct thread *sched_head;
struct thread *sched_tail;

static int schedule_push(struct thread *thread) {

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

static int schedule_remv(struct thread *thread) {
	
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

int event_wait(int id, int event) {
	struct thread *thread = thread_get(id);

	if (!thread) {
		return 1;
	}

	if (event < 0 || event >= EV_COUNT) {
		return 1;
	}

	if (thread->ev_head) {
		/* handle event queued locally for this thread */
		struct event *e = thread->ev_head;
		
		thread->ev_head = e->next;
		if (thread->ev_tail == e) {
			thread->ev_tail = NULL;
		}
		thread->eax = e->event;
		thread->ecx = e->status;
		heap_free(e, sizeof(struct event));

		thread_set_state(thread->id, TS_QUEUED);
	}
	else if (evqueue[event].ev_head) {
		/* handle already-queued event */
		struct event *e = evqueue[event].ev_head;

		evqueue[event].ev_head = e->next;
		if (evqueue[event].ev_tail == e) {
			evqueue[event].ev_tail = NULL;
		}
		thread->eax = event;
		thread->ecx = e->status;
		heap_free(e, sizeof(struct event));

		thread_set_state(thread->id, TS_QUEUED);
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
		thread_set_state(thread->id, TS_WAITING);
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

int event_send(int id, int event, int status) {

	if (event < 0 || event >= EV_COUNT) {
		return 1;
	}
	
	if (id == -1) {

		// send to waiting thread
		if (evqueue[event].front_thread) {
			// handle event now
			struct thread *thread = evqueue[event].front_thread;
			evqueue[event].front_thread = thread->next_evqueue;
			if (!thread->next_evqueue) evqueue[event].back_thread = NULL;
			
			thread->eax = event;
			thread->ecx = status;

			thread->event = -1;
			thread_set_state(thread->id, TS_QUEUED);
		}
		else {
			// queue event
			struct event *ev = heap_alloc(sizeof(struct event));
			ev->status = status;
			ev->event  = event;
			if (!evqueue[event].ev_tail) {
				evqueue[event].ev_tail = ev;
				evqueue[event].ev_head = ev;
				ev->next = NULL;
			}
			else {
				evqueue[event].ev_tail->next = ev;
				evqueue[event].ev_tail = ev;
				ev->next = NULL;
			}
		}
	}
	else {
		struct thread *thread = thread_get(id);

		if (!thread) {
			return 1;
		}

		if (thread_get_state(id) != TS_WAITING) {
			// queue event in local event queue
			struct event *ev = heap_alloc(sizeof(struct event));
			ev->status = status;
			ev->event  = event;
			if (thread->ev_tail) {
				thread->ev_tail = ev;
				thread->ev_head = ev;
				ev->next = NULL;
			}
			else {
				thread->ev_tail->next = ev;
				thread->ev_tail = ev;
				ev->next = NULL;
			}
		}
		else {
			// handle event now
			thread->eax = event;
			thread->ecx = status;

			event_remv(thread->id, thread->event);
			thread->event = -1;
			thread_set_state(thread->id, TS_QUEUED);
		}
	}

	return 0;
}
