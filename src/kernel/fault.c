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

#include "thread.h"
#include "string.h"
#include "debug.h"
#include "space.h"
#include "cpu.h"

/*****************************************************************************
 * fault_generic
 *
 * General fault handler for anything that cannot be handled usefully. If the
 * fault is in userspace, it freezes the current process and sends it a
 * PORT_ILL message. If the fault is in kernelspace, it panics.
 */

void fault_generic(struct thread *image) {

	/* If in kernelspace, panic */
	if (image->cs & 0x3) {
		debug_printf("EIP:%x NUM:%d ERR:%x\n", image->eip, image->num, image->err);
		debug_panic("unknown exception");
	}

	// pause thread
	thread_save(image);
	image->state = TS_PAUSED;
	image->fault = FV_ACCS;

	// add to fault queue
	fault_push(image);
}

/*****************************************************************************
 * fault_page
 *
 * Page fault handler. If the fault is from userspace, and from a TLS block,
 * memory is allocated to fix the issue; if not from a TLS block, the
 * process is frozen and sent a message on port PORT_PAGE. If the fault is
 * from kernel space, it panics.
 *
 * Note: lines that cause a panic with a stack dump on userspace faults are
 * commented out, but can be very useful for tracking userspace bugs, until I
 * write a proper debugger.
 */

void fault_page(struct thread *image) {
	uint32_t cr2;

	/* Get faulting address from register CR2 */
	cr2 = cpu_get_cr2();

	/* If in kernelspace, panic */
	if ((image->cs & 0x3) == 0) { /* i.e. if it was kernelmode */	

		debug_printf("page fault at %x, ip = %x, frame %x, esp = %x\n", 
			cr2, image->eip, page_get(cr2), image->esp);

		debug_panic("page fault exception");
	}

	/* fault */
	thread_save(image);
	image->fault_addr = cr2;
	image->fault = FV_PAGE;
	image->state = TS_PAUSED;

	// add to fault queue
	fault_push(image);
}

/*****************************************************************************
 * fault_double
 *
 * Double fault handler. Double faults can only be the kernel's fault (and
 * shouldn't ever be produced at this point.), so it panics.
 */

struct thread *fault_double(struct thread *image) {

	/* Can only come from kernel problems */
	debug_printf("DS:%x CS:%x\n", image->ds, image->cs);
	debug_panic("double fault exception");
	return NULL;
}
