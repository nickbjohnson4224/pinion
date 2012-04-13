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

#include <config/address.h>
#include <stdbool.h>
#include <stdint.h>

#include "kernel.h"
#include "log.h"

uint32_t stack[65536];

int t_reaper;
int t_debugger;

void func0(void) {
	log(VERBOSE, "func0 on thread %d", gettid());
}

void func1(void) {
	log(VERBOSE, "func1 on thread %d", gettid());

	for(;;);
}

void debugger(void) {
	int thread;
	struct t_info info;

	log(INIT, "debugger starting on thread %d", gettid());

	while (1) {
		int event = wait(EV_FAULT, &thread);

		if (event != EV_FAULT) {
			continue;
		}

		t_info(thread, &info);

		log(DEBUG, "page fault on thread %d at address %p", thread, info.fault_addr);
		log(DEBUG, "EIP: %p", info.regs.eip);
		log(DEBUG, "ESP: %p\tEBP: %p", info.regs.esp, info.regs.ebp);
		log(DEBUG, "EAX: %p\tEBX: %p", info.regs.eax, info.regs.ebx);
		log(DEBUG, "ECX: %p\tEDX: %p", info.regs.ecx, info.regs.edx);
		log(DEBUG, "EDI: %p\tESI: %p", info.regs.edi, info.regs.esi);

		if (info.regs.esi == 1337) {
			log(DEBUG, "debugger killing thread %d", thread);
			kill(thread, 1);
		}
		else {
			info.regs.esi = 1337;
			fixup(thread, &info);
			resume(thread);
		}
	}
}

void reaper(void) {
	int thread;	

	log(INIT, "reaper starting on thread %d", gettid());

	while (1) {
		int event = wait(EV_EXIT, &thread);

		if (event != EV_EXIT) {
			continue;
		}

		reap(thread, (void*) 0);
	}
}

#define ALT  0x00800000
#define CTRL 0x00800001
#define SHFT 0x00800002
#define SYSR 0x00800003
#define WIN	 0x00800004
#define CAPS 0x00800005
#define INS  0x00800006
#define ENTR 0x00800007
#define UP   0x00800008
#define DOWN 0x00800009
#define LEFT 0x0080000A
#define RGHT 0x0080000B
#define PGUP 0x0080000C
#define PGDN 0x0080000D
#define HOME 0x0080000E
#define END	 0x0080000F
#define ESC  0x00800010

#define F1   0x00800011
#define F2   0x00800012
#define F3   0x00800013
#define F4   0x00800014
#define F5   0x00800015
#define F6   0x00800016
#define F7   0x00800017
#define F8   0x00800018
#define F9   0x00800019
#define F10  0x0080001A
#define F11  0x0080001B
#define F12  0x0080001C

#define NUML 0x0080001D
#define SCRL 0x0080001E
#define DEL  0x0080001F

const int keymap[4][128] = {
	{	// lower case, no numlock
		0,
		ESC,  '1',  '2',  '3', '4',  '5', '6',  '7', '8',  '9', '0', '-',  '=', '\b',
		'\t', 'q',  'w',  'e', 'r',  't', 'y',  'u', 'i',  'o', 'p', '[',  ']', '\n', 
		CTRL, 'a',  's',  'd', 'f',  'g', 'h',  'j', 'k',  'l', ';', '\'',  '`', SHFT, 
		'\\', 'z',  'x',  'c', 'v',  'b', 'n',  'm', ',',  '.', '/', SHFT, '*', 
		ALT,  ' ',  CAPS, F1,  F2,   F3,  F4,   F5,  F6,   F7,  F8,  F9,   F10,
		NUML, SCRL, HOME, UP,  PGUP, '-', LEFT, 5,   RGHT, '+', END, DOWN, PGDN, INS, DEL,
		SYSR, 0,    WIN,  F11, F12,  0,   0,    WIN, WIN,
	},
	{	// lower case, numlock
		0,
		ESC,  '1',  '2',  '3', '4', '5', '6', '7', '8', '9', '0', '-',  '=', '\b',
		'\t', 'q',  'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[',  ']', '\n', 
		CTRL, 'a',  's',  'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',  '`', SHFT, 
		'\\', 'z',  'x',  'c', 'v', 'b', 'n', 'm', ',', '.', '/', SHFT, '*',
		ALT,  ' ',  CAPS, F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9,   F10,
		NUML, SCRL, 7,    8,   9,   '-', 4,   5,   6,   '+', 1,   2,    3,   0, '.',
		SYSR, 0,    WIN,  F11, F12, 0,   0,   WIN, WIN,
	},
	{	// upper case, no numlock
		0,
		ESC,  '!',  '@',  '#', '$', '%', '^',  '&', '*',  '(', ')', '_',  '+', '\b',
		'\t', 'Q',  'W',  'E', 'R', 'T', 'Y',  'U', 'I',  'O', 'P', '{',  '}', '\n', 
		CTRL, 'A',  'S',  'D', 'F', 'G', 'H',  'J', 'K',  'L', ':', '"',  '~', SHFT, 
		'|', 'Z',  'X',  'C', 'V', 'B', 'N',  'M', '<',  '>', '?', SHFT, '*', 
		ALT,  ' ',  CAPS, F1,  F2,   F3,  F4,   F5,  F6,   F7,  F8,  F9,   F10,
		NUML, SCRL, HOME, UP,  PGUP, '-', LEFT, 5,   RGHT, '+', END, DOWN, PGDN, INS, DEL,
		SYSR, 0,    WIN,  F11, F12,  0,   0,    WIN, WIN,
	},
	{	// upper case, numlock
		0,
		ESC,  '!',  '@',  '#', '$', '%', '^',  '&', '*',  '(', ')', '_',  '+', '\b',
		'\t', 'Q',  'W',  'E', 'R', 'T', 'Y',  'U', 'I',  'O', 'P', '{',  '}', '\n', 
		CTRL, 'A',  'S',  'D', 'F', 'G', 'H',  'J', 'K',  'L', ':', '"',  '~', SHFT, 
		'|', 'Z',  'X',  'C', 'V', 'B', 'N',  'M', '<',  '>', '?', SHFT, '*', 
		ALT,  ' ',  CAPS, F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9,   F10,
		NUML, SCRL, 7,    8,   9,   '-', 4,   5,   6,   '+', 1,   2,    3,   0, '.',
		SYSR, 0,    WIN,  F11, F12, 0,   0,   WIN, WIN,
	}
};

void keyboard(void) {

	static bool shift = false;
	static bool caps  = false;
	static bool numlk = false;
	
	while (1) {
		int status;
		int event = wait(EV_IRQ(1), &status);

		if (event != EV_IRQ(1)) {
			continue;
		}

		extern uint8_t inb(uint16_t port);
		uint8_t scan = inb(0x60) & 0xFF;
		if (scan == 0xE0) continue;
		int code = keymap[((shift ^ caps) ? 2 : 0) | ((numlk) ? 1 : 0)][scan & ~0x80];

		if (scan & 0x80) {
			switch (code) {
			case SHFT: shift = false; break;
			case NUML: numlk = false; break;
			default:
				log(VERBOSE, "key release\t%c", code);
			}
		}
		else {
			switch (code) {
			case SHFT: shift = true; break;
			case CAPS: caps ^= true; break;
			case NUML: numlk = true; break;
			default:
				log(VERBOSE, "key press\t%c", code);
			}
		}
	}
}

void init(void) {
	uint32_t *foo = (void*) 0x12345000;

	__init_log();	

	t_debugger = spawn(&stack[255], debugger);
	t_reaper = spawn(&stack[511], reaper);
	spawn(&stack[65535], keyboard);

	alloc((uint32_t) foo, 0x3);

	foo[42] = 7;

	*((volatile int*) 42) = 24;

	exit(0);
}
