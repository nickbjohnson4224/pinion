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

#include <pinion.h>
#include "kernel.h"
#include "log.h"

uint32_t stack[65536];

int t_reaper;
int t_debugger;

void func0(void) {
	log(VERBOSE, "func0 on thread %d", __t_getid());
}

void func1(void) {
	log(VERBOSE, "func1 on thread %d", __t_getid());

	for(;;);
}

void debugger(void) {
	struct t_info info;
	
	log(INIT, "debugger starting on thread %d", __t_getid());

	while (1) {
		int thread = __t_getfault();

		__t_getstate(thread, &info);

		const char *name = "nameless";

		log(ERROR, "page fault from %d (%s) at %p", thread, name, info.fault_addr);
		log(ERROR, "EIP: %p", info.regs.eip);
		log(ERROR, "ESP: %p\tEBP: %p", info.regs.esp, info.regs.ebp);
		log(ERROR, "EAX: %p\tEBX: %p", info.regs.eax, info.regs.ebx);
		log(ERROR, "ECX: %p\tEDX: %p", info.regs.ecx, info.regs.edx);
		log(ERROR, "EDI: %p\tESI: %p", info.regs.edi, info.regs.esi);
		log(ERROR, "");

		log(ERROR, "stack trace:");
		for (int i = 6; i >= 0; i--) {
			int val = ((uint32_t*) info.regs.esp)[i];
			log(ERROR, "[ESP+%b]: %p (%d)", i * 4, val, val);
		}

		__t_kill(thread, 1);
	}
}

void reaper(void) {
	log(INIT, "reaper starting on thread %d", __t_getid());

	while (1) {
		int thread = __t_getdead();

		__t_reap(thread, (void*) 0);
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
		__irq_wait(1);

		extern uint8_t inb(uint16_t port);
		uint8_t scan = inb(0x60) & 0xFF;

		__irq_reset(1);

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
	struct t_info state;

	__init_log();

	log(INIT, "starting up");

	state.regs.eip = (uintptr_t) debugger;
	state.regs.esp = (uintptr_t) &stack[255];
	t_debugger = __t_spawn(&state);

	state.regs.eip = (uintptr_t) reaper;
	state.regs.esp = (uintptr_t) &stack[511];
	t_reaper = __t_spawn(&state);

	state.regs.eip = (uintptr_t) keyboard;
	state.regs.esp = (uintptr_t) &stack[65535];
	__t_spawn(&state);

	*((volatile int*) 42) = 24;
}
