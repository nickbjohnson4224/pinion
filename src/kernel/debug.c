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

#include <stdarg.h>
#include <stdint.h>

#include <config/address.h>

#include "debug.h"
#include "ports.h"
#include "cpu.h"

/****************************************************************************
 * __itoa
 *
 * Convert the unsigned integer <n> to a string in base <base> and store in
 * <buffer>. Returns 0 on success, nonzero on error.
 */

void __itoa(char *buffer, size_t n, size_t base) {
	const char *d;
	char temp;
	size_t i, size;

	d = "0123456789ABCDEF";

	if (n == 0) {
		buffer[0] = '0';
		buffer[1] = '\0';
		return;
	}

	if (base > 16) {
		buffer[0] = '\0';
		return;
	}

	for (i = 0; n; i++) {
		buffer[i] = d[n % base];
		n /= base;
	}

	buffer[i] = '\0';
	size = i;

	for (i = 0; i < (size / 2); i++) {
		temp = buffer[size - i - 1];
		buffer[size - i - 1] = buffer[i];
		buffer[i] = temp;
	}

	return;
}

/****************************************************************************
 * debug_printf
 *
 * Prints to debugging output using a similar interface to printf().
 */

void debug_printf(const char *format, ...) {
	size_t i, fbt;
	char buffer[13];
	char fmtbuffer[100];
	const char *str;
	va_list ap;
	char c;

	va_start(ap, format);

	fbt = 0;

	for (i = 0; format[i]; i++) {
		if (format[i] == '%') {
			fmtbuffer[fbt] = '\0';
			debug_string(fmtbuffer);
			fbt = 0;

			switch (format[i+1]) {
			case 'x':
			case 'X':
				__itoa(buffer, va_arg(ap, int), 16);
				debug_string(buffer);
				break;
			case 'd':
			case 'i':
				__itoa(buffer, va_arg(ap, int), 10);
				debug_string(buffer);
				break;
			case 's':
				str = va_arg(ap, const char*);
				if (str) {
					debug_string(str);
				}
				else {
					debug_string("(null)");
				}
				break;
			case 'c':
				c = va_arg(ap, int);
				debug_char(c);
				break;
			case '%':
				c = '%';
				debug_char('%');
				break;
			}
			i++;
		}
		else {
			fmtbuffer[fbt++] = format[i];
		}
	}
	
	fmtbuffer[fbt] = '\0';
	debug_string(fmtbuffer);

	va_end(ap);
}

/****************************************************************************
 * debug_string
 *
 * Prints the string <s> onto debugging output.
 */

void debug_string(const char *s) {
	size_t i;
	
	for (i = 0; s[i]; i++) {
		debug_char(s[i]);
	}
}

/****************************************************************************
 * debug_char
 *
 * Prints a single character <c> onto debugging output. Non-printing 
 * characters, such as tab and backspace, are handled.
 */

void debug_char(char c) {

	while (!(inb(0x3FD) & 0x20));
	outb(0x3F8, c);
	if (c == '\n') debug_char('\r');
}

/****************************************************************************
 * debug_init
 *
 * Initializes debugging output.
 */

void debug_init(void) {

	outb(0x3F9, 0x00);
	outb(0x3FB, 0x80);
	outb(0x3F8, 0x03);
	outb(0x3F9, 0x00);
	outb(0x3FB, 0x03);
	outb(0x3FA, 0xC7);
	outb(0x3FC, 0x0B);
}

void debug_panic(const char *message) {

	debug_printf("Kernel panic: %s\n", message);
	cpu_halt();
}
