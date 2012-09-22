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
#include "ports.h"
#include "debug.h"
#include "cpu.h"

/*****************************************************************************
 * _is_init
 *
 * Set to zero if the interrupt system has not been initialized, nonzero if
 * it has.
 */

static int _is_init;
static void init_idt(void);
static void init_tss(void);

/* interrupt handling *******************************************************/

/*****************************************************************************
 * _int_handler
 *
 * Array of registered interrupt handlers. Modified exclusively by 
 * int_set_handler(), and accessed exclusively by int_handler().
 */

static int_handler_t _int_handler[256];

/*****************************************************************************
 * int_set_handler
 *
 * Sets the interrupt with id <n> to call <handler> when fired.
 */

void int_set_handler(intid_t n, int_handler_t handler) {

	if (!_is_init) {
		init_idt();
		_is_init = 1;
	}

	_int_handler[n] = handler;
}

/*****************************************************************************
 * int_handler
 *
 * Handles all interrupts, redirecting them to the proper interrupt handlers.
 * This function is only called by the state-saving assembly routine in
 * "int.s", which is why it's not in any header files but non-static.
 */

struct thread *int_handler(struct thread *image) {

	/* reset IRQs if it was an IRQ */
	if (ISIRQ(image->num)) {

		if (INT2IRQ(image->num) != 0) {
			event_send(-1, INT2IRQ(image->num));
		}
		else {
			irq_reset(INT2IRQ(image->num));
		}
	}

	/* call registered interrupt handler */
	if (_int_handler[image->num]) {
		_int_handler[image->num](image);
	}

	/* return active thread */
	image = thread_get_active();
	if (!image) {

		/* get next thread from scheduler */
		image = schedule_next();
		if (!image) {
			cpu_idle();
		}

		/* schedule new active thread */
		schedule_remv(image);
		thread_load(image);
		image->state = TS_RUNNING;
	}

	return image;
}

/* IDT driver ***************************************************************/

/*****************************************************************************
 * idt
 *
 * The "Interrupt Descriptor Table" also known as the IDT. This is the part
 * of the x86 architecture that redirects interrupts to interrupt handlers.
 * These are the low-level assembly handlers (NOT the C handlers registered by
 * int_set_handler()).
 */

static struct idt idt[256];

/* Assembly interrupt handler stubs to be registered in the IDT *************/

extern void
	int0(void),  int1(void),  int2(void),  int3(void), 
	int4(void),  int5(void),  int6(void),  int7(void),  
	int8(void),  int9(void),  int10(void), int11(void), 
	int12(void), int13(void), int14(void), int15(void), 
	int16(void), int17(void), int18(void), int19(void),

	int32(void), int33(void), int34(void), int35(void), 
	int36(void), int37(void), int38(void), int39(void), 
	int40(void), int41(void), int42(void), int43(void), 
	int44(void), int45(void), int46(void), int47(void),

	int129(void);

/*****************************************************************************
 * idt_raw
 *
 * Array of low-level interrupt handlers to be mirrored by the IDT. This
 * array is used to generate the IDT when init_int_handler() is called, and
 * is never used again.
 */

typedef void (*int_raw_handler_t) (void);
static int_raw_handler_t idt_raw[64] = {

	/* faults */
	int0, 	int1, 	int2, 	int3, 	int4, 	int5, 	int6, 	int7, 
	int8, 	int9, 	int10, 	int11, 	int12, 	int13, 	int14, 	int15, 
	int16,	int17, 	int18, 	int19, 	NULL, 	NULL, 	NULL, 	NULL, 
	NULL, 	NULL, 	NULL, 	NULL, 	NULL, 	NULL, 	NULL, 	NULL, 

	/* IRQs */
	int32,	int33, 	int34, 	int35, 	int36, 	int37, 	int38, 	int39, 
	int40,	int41, 	int42, 	int43, 	int44, 	int45, 	int46, 	int47,
	NULL, 	NULL, 	NULL, 	NULL, 	NULL, 	NULL, 	NULL, 	NULL, 
	NULL, 	NULL, 	NULL, 	NULL, 	NULL, 	NULL, 	NULL, 	NULL, 

};

/****************************************************************************
 * idt_set
 *
 * Set the interrupt descriptor <n> to handler <base> in code segment <seg>
 * with flags <flags>. See the Intel Manuals for details on this.
 */

static void idt_set(intid_t n, uint32_t base, uint16_t seg, uint8_t flags) {

	if (!base) {
		return; /* Ignore null handlers */
	}
	
	idt[n].base_l = (uint16_t) (base & 0xFFFF);
	idt[n].base_h = (uint16_t) (base >> 16);
	idt[n].seg = seg;
	idt[n].reserved = 0;
	idt[n].flags = flags;
}

/****************************************************************************
 * init_idt
 *
 * Initialize the interrupt handling system.
 */

static void init_idt(void) {
	size_t i;

	/* Write privileged interrupt handlers (faults, IRQs) */
	for (i = 0; i < 64; i++) {
		if (idt_raw[i]) {
			idt_set(i, (uint32_t) idt_raw[i], 0x08, 0x8E);
		}
	}

	/* Write usermode interrupt handlers (syscalls) */
	idt_set(129, (uint32_t) int129, 0x08, 0xEE);

	/* Write the IDT */
	cpu_set_idt(idt);

	/* setup the TSS */
	init_tss();
}

/* TSS driver ***************************************************************/

/*****************************************************************************
 * tss
 *
 * The "Task State Segment" also known as the TSS. This is part of the x86
 * architecture that can be used for hardware task switching, but we're only
 * using it to set the interrupt handler stack.
 */

static struct tss tss;

/*****************************************************************************
 * gdt
 *
 * The "Global Descriptor Table" also known as the GDT. This is the part of 
 * the x86 architecture that contains all of the protected mode segment 
 * descriptors. Each descriptor is actually 8 bytes in length, but we're
 * interpreting it as an array of bytes for simplicity. The TSS is the sixth
 * descriptor.
 *
 * The actual gdt is defined in "kernel/boot.s".
 */

extern uint8_t gdt[64];

/*****************************************************************************
 * init_tss
 *
 * Initializes the system responsible for set_int_stack(). On the x86, this
 * effectively means initializing the TSS, which is responsible for usermode
 * to kernelmode switches.
 */

static void init_tss(void) {
	uint32_t base = (uint32_t) &tss;
	uint16_t limit = (uint16_t) (base + sizeof(struct tss) - 1);

	memclr(&tss, sizeof(struct tss));
	tss.cs = 0x08;
	tss.ss0 = tss.es = tss.ds = tss.fs = tss.gs = 0x10;
	tss.iomap_base = 104;

	/* Change the 8th GDT entry to be the TSS */
	gdt[56] = (uint8_t) ((limit) & 0xFF);
	gdt[57] = (uint8_t) ((limit >> 8) & 0xFF);
	gdt[58] = (uint8_t) (base & 0xFF);
	gdt[59] = (uint8_t) ((base >> 8) & 0xFF);
	gdt[60] = (uint8_t) ((base >> 16) & 0xFF);
	gdt[63] = (uint8_t) ((base >> 24) & 0xFF);

	cpu_sync_tss();
}

/*****************************************************************************
 * set_int_stack
 *
 * Sets the pointer at which the stack will start when an interrupt is
 * handled. This stack is where the thread state is saved, not the global
 * kernel stack: in general, it should always be set to &thread->kernel_stack 
 * for the currently running thread.
 */

void set_int_stack(void *ptr) {
	tss.esp0 = (uintptr_t) ptr;
}

/* 8259 PIC driver **********************************************************/

/*****************************************************************************
 * irq_init
 *
 * Initialize the PIC (Programmable Interrupt Controller).
 */

static uint16_t _irq_mask;

int irq_init(void) {

	/* (re)initialize 8259 PIC */
	outb(0x20, 0x11); /* Initialize master */
	outb(0xA0, 0x11); /* Initialize slave */
	outb(0x21, 0x20); /* Master mapped to 0x20 - 0x27 */
	outb(0xA1, 0x28); /* Slave mapped to 0x28 - 0x2E */
	outb(0x21, 0x04); /* Master thingy */
	outb(0xA1, 0x02); /* Slave thingy */
	outb(0x21, 0x01); /* 8086 (standard) mode */
	outb(0xA1, 0x01); /* 8086 (standard) mode */
	outb(0x21, 0x00); /* Master IRQ mask */
	outb(0xA1, 0x00); /* Slave IRQ mask */

	return 0;
}

int irq_mask(irqid_t irq) {
	_irq_mask |= (1 << irq);

	if (irq > 7) outb(0xA1, _irq_mask >> 8);
	else         outb(0x21, _irq_mask & 0xFF);

	return 0;
}

int irq_unmask(irqid_t irq) {
	_irq_mask &= ~(1 << irq);

	if (irq > 7) outb(0xA1, _irq_mask >> 8);
	else         outb(0x21, _irq_mask & 0xFF);

	return 0;
}

/*****************************************************************************
 * irq_reset
 *
 * Resets IRQs after an IRQ is handled. <irq> is the specific IRQ that was
 * handled. Assumes that there actually was a valid IRQ generated. Returns
 * zero on success, nonzero on failure.
 *
 * Note: this exists mostly to cater to the 8259 PIC's requirements. After it
 * generates an IRQ, it has to be reset, and the way it needs to be reset
 * depends on which IRQ was generated. This function may not need <irq> or may
 * not be needed at all on non-PC systems or systems with an APIC.
 */

int irq_reset(irqid_t irq) {
	
	if (irq > IRQ_INT_SIZE) {
		return 1;
	}

	if (irq > 8) {
		outb(0xA0, 0x20);
	}
	outb(0x20, 0x20);

	return 0;
}

/* 8253 PIT driver **********************************************************/

static uint32_t pit_freq = 1;

/*****************************************************************************
 * timer_handler (interrupt handler)
 *
 * Increments the timer tick and preempts the current thread.
 */

static void timer_handler(struct thread *image) {
	static uint32_t tick = 0;

	tick++;

	// trigger virtual event timers
	uint32_t ptime = ((tick - 1) << 10) / pit_freq;
	uint32_t ctime = (tick << 10) / pit_freq;
	uint32_t time = (ptime ^ ctime);
	time = time + (time & -time) - 1;

	for (int i = 0; i < 16; i++) {
		if (time & (1 << (15 - i))) {
			event_send(-1, EV_VTIMER(i));
		}
	}

	image->tick++;

	if (image->state == TS_RUNNING) {
		thread_save(image);
		schedule_push(image);
		image->state = TS_QUEUED;
	}
}

/******************************************************************************
 * timer_set_freq
 *
 * Sets the timer to fire with a frequency of <hertz> hertz (approximately). If
 * the timer has not been initialized, it is done now. Returns zero on success,
 * nonzero on failure (i.e. if the given frequency is impossible to obtain).
 */

int timer_set_freq(uint32_t hertz) {
	uint32_t divider;

	divider = 1193180 / hertz;

	if ((divider == 0) || (divider >= 65536)) {
		return 1;
	}

	outb(0x43, 0x36);
	outb(0x40, (uint8_t) (divider & 0xFF));
	outb(0x40, (uint8_t) (divider >> 0x8));

	irq_init();
	int_set_handler(IRQ2INT(0), timer_handler);

	pit_freq = hertz;

	return 0;
}
