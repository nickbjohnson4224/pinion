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

#include "syscall.h"
#include "string.h"
#include "space.h"
#include "thread.h"
#include "debug.h"
#include "pctx.h"

static void save_info(struct t_info *dest, struct thread *src);
//static void load_info(struct thread *dest, struct t_info *src);

void kcall(struct thread *image) {

	if (image->flags & TF_USER) {
		// perform syscall

		// save user state
		image->usr_eip = image->eip;
		image->usr_esp = image->useresp;

		// switch to system mode
		image->ss = 0x21;
		image->ds = 0x21;
		image->cs = 0x19;
		image->flags &= ~TF_USER;

		// restore system state
		image->eip = image->sys_eip;
		image->useresp = image->sys_esp;		

		return;
	}

	switch (image->eax) {

	case KCALL_SPAWN: {

		int id = thread_new();
		if (id == -1) {
			// failure to create new thread
			image->eax = -1;
			break;
		}

		struct thread *thread = thread_get(id);
		struct t_info *state = (void*) image->ebx;

		// initialize thread state
		thread->useresp = state->regs.esp;
		thread->esp     = (uintptr_t) &thread->num;
		thread->ss      = 0x21;
		thread->ds      = 0x21;
		thread->cs      = 0x19;
		thread->eflags  = image->eflags;
		thread->eip     = state->regs.eip;
		thread->ebp     = state->regs.ebp;
		thread->esi     = state->regs.esi;
		thread->edi     = state->regs.edi;
		thread->edx     = state->regs.edx;
		thread->ecx     = state->regs.ecx;
		thread->ebx     = state->regs.ebx;
		thread->eax     = state->regs.eax;

		// add to scheduler queue
		thread->state = TS_QUEUED;
		schedule_push(thread);

		image->eax = id;
		break;
	}

	case KCALL_REAP: {

		struct thread *target = thread_get(image->ebx);

		if (target->state != TS_PAUSED) {
			image->eax = TE_STATE;
		}
		else {

			if (image->ecx) {
				save_info((void*) image->ecx, target);
			}

			image->eax = 0;
			thread_kill(target);
		}

		break;
	}

	case KCALL_GETTID: {

		image->eax = image->id;

		break;
	}

	case KCALL_YIELD: {
		
		thread_save(image);
		schedule_push(image);
		image->state = TS_QUEUED;

		break;
	}

	case KCALL_PAUSE: {

		struct thread *target = thread_get(image->ebx);
		if (image->ebx == (uint32_t) -1) target = image;

		if (!target) {
			image->eax = TE_EXIST;
		}
		else switch (target->state) {
		case TS_RUNNING:

			// pause (normal, from running)
			thread_save(target);
			target->state = TS_PAUSED;
			
			image->eax = 0;
			break;

		case TS_QUEUED:

			// pause (normal, from queued)
			schedule_remv(target);
			target->state = TS_PAUSED;

			image->eax = 0;
			break;

		case TS_WAITING:

			// pause (waiting)
			event_remv(target->id, target->event);
			target->state = TS_PAUSEDW;

			image->eax = 0;
			break;

		default:

			// invalid state transition
			image->eax = TE_STATE;
			break;
		}

		break;
	}

	case KCALL_RESUME: {

		struct thread *target = thread_get(image->ebx);
		if (!target) {
			image->eax = TE_EXIST;
		}
		else switch (target->state) {
		case TS_PAUSED:

			// resume thread by scheduling
			schedule_push(target);
			target->state = TS_QUEUED;

			image->eax = 0;
			break;

		case TS_PAUSEDW:

			// resume thread by entering wait queue
			event_wait(target->id, target->event);
			target->state = TS_WAITING;

			image->eax = 0;
			break;

		default:

			image->eax = TE_STATE;
			break;
		}

		break;
	}

	case KCALL_GETSTATE: {

		struct thread *target = thread_get(image->ebx);
		if (image->ebx == (uint32_t) -1) target = image;

		if (!target) {
			image->eax = TE_EXIST;
		}
		else if (target->state != TS_PAUSED && target->state != TS_PAUSEDW && target != image) {
			image->eax = TE_STATE;
		}
		else {
			save_info((void*) image->ecx, target);
			image->eax = 0;
		}
		
		break;
	}

	case KCALL_SETSTATE: {

		struct thread *target = thread_get(image->ebx);
		if (image->ebx == (uint32_t) -1) target = image;

		if (!target) {
			image->eax = TE_EXIST;
		}
		else switch (target->state) {
		case TS_PAUSED:
		case TS_PAUSEDW:
		case TS_RUNNING: {

			struct t_info *src = (void*) image->ecx;

			if (src->flags & TF_DEAD) {

				// kill thread
				if (target->state == TS_RUNNING) {
					thread_save(target);
					target->state = TS_PAUSED;
				}

				// notify reaper
				dead_push(target);
			}

			if (target->pctx != src->pctx) {

				// change paging contexts
				pctx_load(src->pctx);
			}

			// save thread state
			target->pctx  = src->pctx;
			target->flags = src->flags;
			target->fault = src->fault;

			if (target->state != TS_RUNNING) {

				// save register state
				target->edi = src->regs.edi;
				target->esi = src->regs.esi;
				target->ebp = src->regs.ebp;
				target->useresp = src->regs.esp;
				target->ebx = src->regs.ebx;
				target->edx = src->regs.edx;
				target->ecx = src->regs.ecx;
				target->eax = src->regs.eax;
				target->eip = src->regs.eip;
				target->eflags = src->regs.eflags;

				// save MMX/SSE state
				if (!target->fxdata) target->fxdata = heap_alloc(512);
				memcpy(target->fxdata, &src->regs.fxdata[0], 512);
			}

			target->usr_eip = src->usr_ip;
			target->usr_esp = src->usr_sp;

			image->eax = 0;
			break;
		}

		default:

			image->eax = TE_STATE;
			break;
		}

		break;
	}

	case KCALL_GETDEAD: {

		if (dead_peek()) {
			struct thread *dead = dead_pull();
			image->eax = dead->id;
		}
		else {
			thread_save(image);
			image->state = TS_PAUSED;

			dead_wait(image);
		}

		break;
	}

	case KCALL_GETFAULT: {

		if (fault_peek()) {
			struct thread *fault = fault_pull();
			image->eax = fault->id;
		}
		else {
			thread_save(image);
			image->state = TS_PAUSED;

			fault_wait(image);
		}

		break;
	}

	case KCALL_WAIT: {
		
		image->eax = event_wait(image->id, image->ebx);

		break;

	}

	case KCALL_RESET: {

		if (image->ebx < 240) {
			extern int irqstate[EV_COUNT];

			irqstate[image->ebx] = 0;
			irq_unmask(image->ebx);
		}

		image->eax = 0;

		break;
	}

	case KCALL_SYSRET: {

		// save system state
		image->sys_eip = image->eip;
		image->sys_esp = image->useresp;
		
		// perform return value swap-in
		image->eax = image->ebp;

		// switch to usermode
		image->ss = 0x33;
		image->ds = 0x33;
		image->cs = 0x2B;
		image->flags |= TF_USER;

		// restore user state
		image->eip = image->usr_eip;
		image->useresp = image->usr_esp;

		break;
	}

	case KCALL_NEWPCTX: {

		image->eax = pctx_new();

		break;
	}

	case KCALL_FREEPCTX: {

		image->eax = pctx_free(image->ebx);

		break;
	}


	case KCALL_SETFRAME: {

		page_set(image->ebx, page_fmt(image->ecx, page_get(image->ebx)));
		image->eax = 0;

		break;
	}

	case KCALL_SETFLAGS: {

		page_set(image->ebx, page_fmt(page_ufmt(page_get(image->ebx)), image->ecx));
		image->eax = 0;

		break;
	}

	case KCALL_GETFRAME: {

		uint32_t off  = image->ebx & 0xFFF;
		uint32_t page = image->ebx & ~0xFFF;
		image->eax = page_ufmt(page_get(page)) | off;

		break;
	}

	case KCALL_GETFLAGS: {

		image->eax = page_get(image->ebx) & PF_MASK;

		break;
	}

	case KCALL_NEWFRAME: {

		uint64_t frame = frame_new();
		image->eax = frame & 0xFFFFFFFF;
		image->ebx = frame >> 32ULL;

		break;
	}

	case KCALL_FREEFRAME: {

		frame_free(image->ebx);

		image->eax = 0;
		break;
	}

	case KCALL_TAKEFRAME: {
		
		image->eax = 1;
		break;
	}	

	default: {

		debug_printf("warning: unimplemented kcall %d\n", image->eax);
		image->eax = -1;

		break;
	}

	}
}

static void save_info(struct t_info *dest, struct thread *src) {
	
	dest->id    = src->id;
	dest->pctx  = src->pctx;
	dest->state = src->state;
	dest->event = src->event;
	dest->flags = src->flags;

	dest->fault = src->fault;
	dest->fault_addr = src->fault_addr;

	if (src->state != TS_RUNNING) {

		dest->regs.edi = src->edi;
		dest->regs.esi = src->esi;
		dest->regs.ebp = src->ebp;
		dest->regs.esp = src->useresp;
		dest->regs.ebx = src->ebx;
		dest->regs.edx = src->edx;
		dest->regs.ecx = src->ecx;
		dest->regs.eax = src->eax;
		dest->regs.eip = src->eip;
		dest->regs.eflags = src->eflags;
	
		if (src->fxdata) {
			memcpy(&dest->regs.fxdata[0], src->fxdata, 512);
		}
	}

	dest->usr_ip = src->usr_eip;
	dest->usr_sp = src->usr_esp;
}
