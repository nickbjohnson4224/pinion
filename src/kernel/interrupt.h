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

#ifndef KERNEL_INTERRUPT_H
#define KERNEL_INTERRUPT_H

#include <stdint.h>
#include "thread.h"
#include "types.h"

/* interrupt handling *******************************************************/

typedef void (*int_handler_t) (struct thread *);
void int_set_handler(intid_t n, int_handler_t handler);

/* interrupt stack **********************************************************/

void set_int_stack(void *ptr);

/* IRQ numbering macros *****************************************************/

#define IRQ_INT_BASE 32
#define IRQ_INT_SIZE 16

#define IRQ2INT(n) ((n) + IRQ_INT_BASE)
#define INT2IRQ(n) ((n) - IRQ_INT_BASE)
#define ISIRQ(n) ((n) >= IRQ_INT_BASE && (n) < (IRQ_INT_BASE + IRQ_INT_SIZE))

/* IRQ initialization *******************************************************/

int irq_init(void);

/* IRQ reset ****************************************************************/

int irq_reset(irqid_t irq);

/* IRQ masking **************************************************************/

int irq_mask(irqid_t irq);
int irq_unmask(irqid_t irq);

/* timer frequency **********************************************************/

int timer_set_freq(uint32_t hertz);

#endif/*KERNEL_INTERRUPT_H*/
