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

#include <config/address.h>

#include "interrupt.h"
#include "syscall.h"
#include "thread.h"
#include "string.h"
#include "fault.h"
#include "space.h"
#include "debug.h"
#include "cpu.h"
#include "elf.h"

/* multiboot structures *****************************************************/

struct multiboot {
	uint32_t flags;
	uint32_t mem_lower;
	uint32_t mem_upper;
	uint32_t boot_device;
	uint32_t cmdline;
	uint32_t mods_count;
	uint32_t mods_addr;
	uint32_t num;
	uint32_t size;
	uint32_t addr;
	uint32_t shndx;
	uint32_t mmap_length;
	uint32_t mmap_addr;
	uint32_t drives_length;
	uint32_t drives_addr;
	uint32_t config_table;
	uint32_t boot_loader_name;
	uint32_t apm_table;
	uint32_t vbe_control_info;
	uint32_t vbe_mode_info;
	uint32_t vbe_mode;
	uint32_t vbe_interface_seg;
	uint32_t vbe_interface_off;
	uint32_t vbe_interface_len;
}  __attribute__((packed));

struct module {
	uint32_t mod_start;
	uint32_t mod_end;
	uint32_t string;
	uint32_t reserved;
} __attribute__ ((packed));

struct memory_map {
	uint32_t size;
	uint32_t base_addr_low;
	uint32_t base_addr_high;
	uint32_t length_low;
	uint32_t length_high;
	uint32_t type;
} __attribute__ ((packed));

/*****************************************************************************
 * init
 *
 * Initializes the kernel completely. Really, what did you expect it to do?
 * Returns the thread to "resume" (i.e. start from).
 *
 * This function is only called from "boot.s". It's not declared in any header
 * files, but don't make it static.
 */

struct thread *init(struct multiboot *mboot, uint32_t mboot_magic) {

	/* initialize debugging output */
	debug_init();
	debug_printf("Pinion-x86 booting...\n");

	/* check multiboot header */
	if (mboot_magic != 0x2BADB002) {
		debug_panic("bootloader is not multiboot compliant");
	}

	/* touch pages for the kernel and system layers */
	for (uint32_t i = 0; i < KERNEL_ADDR_SIZE; i += (1 << 22)) {
		page_touch(KERNEL_ADDR_BASE + i);
	}
	for (uint32_t i = 0; i < SYSTEM_ADDR_SIZE; i += (1 << 22)) {
		page_touch(SYSTEM_ADDR_SIZE + i);
	}

	/* identity map kernel boot frames */
	for (uint32_t i = KERNEL_ADDR_BASE; i < KERNEL_ADDR_BASE + KERNEL_BOOT_SIZE; i += PAGESZ) {
		page_set(i, page_fmt(i - KERNEL_ADDR_BASE, PF_PRES | PF_RW));
	}

	/* parse the multiboot memory map to find the size of memory */
	struct memory_map *mem_map = (void*) (mboot->mmap_addr + KERNEL_ADDR_BASE);
	size_t mem_map_count       = mboot->mmap_length / sizeof(struct memory_map);
	uint32_t memsize = 0;
	for (size_t i = 0; i < mem_map_count; i++) {
		if (mem_map[i].type == 1) {

			// avoid boot frames
			if (mem_map[i].base_addr_low < KERNEL_BOOT_SIZE) {
				if (mem_map[i].length_low < KERNEL_BOOT_SIZE - mem_map[i].base_addr_low) {
					continue;
				}

				mem_map[i].length_low  -= (KERNEL_BOOT_SIZE - mem_map[i].base_addr_low);
				mem_map[i].base_addr_low = KERNEL_BOOT_SIZE;
			}

			debug_printf("found free memory region [%x ... %x]\n", 
				mem_map[i].base_addr_low,
				mem_map[i].base_addr_low + mem_map[i].length_low - 1);

			for (uint32_t addr = 0; addr < mem_map[i].length_low; addr += PAGESZ) {
				frame_add(mem_map[i].base_addr_low + addr);
				memsize += PAGESZ;
			}
		}
	}
	debug_printf("%d MB of memory available\n", memsize >> 20);

	/* get multiboot module information */
	if (mboot->mods_count < 1) debug_panic("no system image found");
	struct module *module           = (void*) (mboot->mods_addr + KERNEL_ADDR_BASE);
	struct elf32_ehdr *system_image = (void*) (module[0].mod_start + KERNEL_ADDR_BASE);

	/* load system */
	if (elf_check_file(system_image)) {
		debug_panic("system is not a valid ELF executable");
	}
	elf_load_file(system_image);

	/* bootstrap first thread */
	int init_thread_id = thread_new();
	struct thread *init_thread = thread_get(init_thread_id);
	static int init_stack[1024];
	init_thread->useresp = (uintptr_t) &init_stack[1023];
	init_thread->esp     = (uintptr_t) &init_thread->num;
	init_thread->ss      = 0x21;
	init_thread->ds      = 0x21;
	init_thread->cs      = 0x19;
	init_thread->eflags  = cpu_get_eflags() | 0x1200; /* IF, IOPL = 1 */
	init_thread->eip     = system_image->e_entry;

	/* register kernel calls */
	int_set_handler(0x81, kcall);

	/* register fault handlers */
	int_set_handler(FAULT_DE, fault_generic);
	int_set_handler(FAULT_DB, fault_generic);
	int_set_handler(FAULT_NI, fault_generic);
	int_set_handler(FAULT_BP, fault_generic);
	int_set_handler(FAULT_OF, fault_generic);
	int_set_handler(FAULT_BR, fault_generic);
	int_set_handler(FAULT_UD, fault_generic);
	int_set_handler(FAULT_NM, fault_generic);
	int_set_handler(FAULT_DF, fault_double);
	int_set_handler(FAULT_CO, fault_generic);
	int_set_handler(FAULT_TS, fault_generic);
	int_set_handler(FAULT_NP, fault_generic);
	int_set_handler(FAULT_SS, fault_generic);
	int_set_handler(FAULT_GP, fault_generic);
	int_set_handler(FAULT_PF, fault_page);
	int_set_handler(FAULT_MF, fault_generic);
	int_set_handler(FAULT_AC, fault_generic);
	int_set_handler(FAULT_MC, fault_generic);
	int_set_handler(FAULT_XM, fault_generic);

	/* start timer (for preemption) */
	timer_set_freq(64);

	/* initialize FPU/MMX/SSE */
	cpu_init_fpu();

	/* drop to usermode, scheduling the next thread */
	debug_printf("passing control to system\n");

	thread_load(init_thread);
	init_thread->state = TS_RUNNING;
	return init_thread;
}
