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
static void load_info(struct thread *dest, struct t_info *src);

void kcall(struct thread *image) {

	switch (image->eax) {

	case KCALL_SPAWN: {

		int id = thread_new();
		struct thread *thread = thread_get(id);
		thread->useresp = image->ebx;
		thread->esp     = (uintptr_t) &thread->num;
		thread->ss      = 0x21;
		thread->ds      = 0x21;
		thread->cs      = 0x19;
		thread->eflags  = image->eflags;
		thread->eip     = image->ecx;

		thread_set_state(thread->id, TS_QUEUED);
		image->eax = id;

		break;
	}

	case KCALL_EXIT: {

		thread_set_state(image->id, TS_ZOMBIE);
		image->exit_status = image->ebx;

		event_send(-1, EV_EXIT, image->id);

		break;
	}

	case KCALL_KILL: {
		
		struct thread *target = thread_get(image->ebx);
		if (!target) {
			image->eax = -1;
		}
		else {
			thread_set_state(target->id, TS_ZOMBIE);
			target->exit_status = image->ecx;
			image->eax = 0;

			event_send(-1, EV_EXIT, target->id);
		}

		break;
	}

	case KCALL_REAP: {
		
		if (thread_get_state(image->ebx) != TS_ZOMBIE) {
			image->eax = 0;
		}
		else {
			struct thread *target = thread_get(image->ebx);

			if (image->ecx) {
				save_info((void*) image->ecx, target);
			}

			image->eax = target->exit_status;
			thread_set_state(image->ebx, TS_FREE);
		}

		break;
	}

	case KCALL_WAIT: {

		image->eax = event_wait(image->id, image->ebx);

		break;
	}

	case KCALL_GETTID: {

		image->eax = image->id;

		break;
	}

	case KCALL_YIELD: {
		
		thread_set_state(image->id, TS_QUEUED);

		break;
	}

	case KCALL_WAKEUP: {

		image->eax = event_send(image->ebx, image->ecx, image->edx);

		break;
	}

	case KCALL_RENICE: {

		// TODO - priorities in scheduler
		break;
	}

	case KCALL_PAUSE: {

		struct thread *target = thread_get(image->ebx);
		if (!target) {
			image->eax = 1;
		}
		else if (thread_get_state(target->id) == TS_RUNNING
			|| thread_get_state(target->id) == TS_QUEUED
			|| thread_get_state(target->id) == TS_WAITING) {

			thread_set_state(target->id, TS_PAUSED);
			image->eax = 0;
		}
		else {
			image->eax = 1;
		}

		break;
	}

	case KCALL_RESUME: {

		struct thread *target = thread_get(image->ebx);
		if (!target) {
			image->eax = 1;
		}
		else if (thread_get_state(target->id) == TS_PAUSED) {
			if (target->event == -1) {
				thread_set_state(target->id, TS_QUEUED);
			}
			else {
				event_wait(target->id, target->event);
			}
			image->eax = 0;
		}
		else {
			image->eax = 1;
		}

		break;
	}

	case KCALL_INFO: {

		struct thread *target = thread_get(image->ebx);
		if (!target) {
			image->eax = 1;
		}
		else {
			save_info((void*) image->ecx, target);
			image->eax = 0;
		}
		
		break;
	}

	case KCALL_FIXUP: {

		struct thread *target = thread_get(image->ebx);
		if (!target) {
			image->eax = 1;
		}
		else {
			load_info(target, (void*) image->ecx);
			image->eax = 0;
		}

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

	case KCALL_SETPCTX: {

		struct thread *target = thread_get(image->ebx);
		if (!target) {
			image->eax = 1;
		}
		else {
			target->pctx = image->ecx;
		}

		break;
	}

	case KCALL_GETPCTX: {

		struct thread *target = thread_get(image->ebx);
		if (!target) {
			image->eax = -1;
		}
		else {
			image->eax = target->pctx;
		}

		break;
	}

	case KCALL_ALLOC: {
	
		page_set(image->ebx, page_fmt(frame_new(), PF_PRES | image->ecx));
		image->eax = 0;

		break;
	}

	case KCALL_SETFRAME: {

		if (page_ufmt(page_get(image->ebx))) {
			frame_free(page_ufmt(page_get(image->ebx)));
		}
		page_set(image->ebx, page_fmt(image->ecx, page_get(image->ebx)));
		if (image->ecx) {
			frame_ref(image->ecx);
		}
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

	default: {

		debug_printf("warning: unimplemented kcall %d\n", image->eax);
		image->eax = -1;

		break;
	}

	}
}

static void save_info(struct t_info *dest, struct thread *src) {
	
	dest->id = src->id;
	dest->state = src->state;
	dest->exit_status = src->exit_status;
	dest->tick = src->tick;
	dest->wait_event = src->event;

	dest->fault_addr = src->pfault_addr;
	dest->fault_type = src->num;
	dest->fault_code = src->err;

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

static void load_info(struct thread *dest, struct t_info *src) {

	dest->exit_status = src->exit_status;
	
	dest->edi = src->regs.edi;
	dest->esi = src->regs.esi;
	dest->ebp = src->regs.ebp;
	dest->useresp = src->regs.esp;
	dest->ebx = src->regs.ebx;
	dest->edx = src->regs.edx;
	dest->ecx = src->regs.ecx;
	dest->eax = src->regs.eax;
	dest->eip = src->regs.eip;
	dest->eflags = src->regs.eflags;

	if (!dest->fxdata) {
		dest->fxdata = heap_alloc(512);
	}
	memcpy(dest->fxdata, &src->regs.fxdata[0], 512);

}
