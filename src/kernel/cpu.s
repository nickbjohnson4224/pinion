; Copyright (C) 2009-2012 Nick Johnson <nickbjohnson4224 at gmail.com>
; 
; Permission to use, copy, modify, and distribute this software for any
; purpose with or without fee is hereby granted, provided that the above
; copyright notice and this permission notice appear in all copies.
; 
; THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
; WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
; ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
; WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
; ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
; OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

[bits 32]

section .bss

idle_stack: resb 0x1000

section .data

idt_ptr:
	dw 0x7FF
	dd 0

section .text

global cpu_clr_ts
cpu_clr_ts:
	clts
	ret

global cpu_flush_tlb_full
cpu_flush_tlb_full:
	mov eax, cr3
	mov cr3, eax
	ret

global cpu_flush_tlb_part
cpu_flush_tlb_part:
	mov eax, [esp+4]
	invlpg [eax]
	ret

global cpu_get_cr2
cpu_get_cr2:
	mov eax, cr2
	ret

global cpu_get_cr3
cpu_get_cr3:
	mov eax, cr3
	ret

global cpu_get_eflags
cpu_get_eflags:
	pushf
	pop eax
	ret

global cpu_get_id
cpu_get_id:
	push ebx
	mov eax, [esp+8]
	cpuid
	mov eax, edx
	pop ebx
	ret

global cpu_get_tsc
cpu_get_tsc:
	rdtsc
	ret

global cpu_halt
cpu_halt:
	cli
	hlt

global cpu_idle
cpu_idle:
	mov esp, idle_stack + 0xFF0
	sti
	hlt

global cpu_set_cr3
cpu_set_cr3:
	mov eax, [esp+4]
	mov cr3, eax
	ret

global cpu_set_idt
cpu_set_idt:
	mov eax, [esp+4]
	lea edx, [idt_ptr+2]
	mov [edx], eax

	mov eax, idt_ptr
	lidt [eax]

	ret

global cpu_set_stack
cpu_set_stack:
	mov eax, [esp+4]
	mov ecx, [esp]
	mov esp, eax
	push ecx
	ret

global cpu_sync_tss
cpu_sync_tss:
	mov ax, 0x38
	ltr ax
	ret

global inb
inb:
	mov dx, [esp+4]
	xor eax, eax
	in al, dx
	ret

global inw
inw:
	mov dx, [esp+4]
	xor eax, eax
	in ax, dx
	ret

global ind
ind:
	mov dx, [esp+4]
	in eax, dx
	ret

global outb
outb:
	mov dx, [esp+4]
	mov al, [esp+8]
	out dx, al
	ret

global outw
outw:
	mov dx, [esp+4]
	mov ax, [esp+8]
	out dx, ax
	ret

global outd
outd:
	mov dx, [esp+4]
	mov eax, [esp+8]
	out dx, eax
	ret
