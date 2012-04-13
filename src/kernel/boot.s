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

; Multiboot stuff
FLAGS       equ  0x03
MAGIC       equ  0x1BADB002
CHECKSUM    equ  -(MAGIC + FLAGS)

section .bss

KSTACKSIZE equ 0x1FF0
global kstack
kstack:
	resb 0x2000

section .pdata

KSPACE equ 0xF0000000

; Initial kernel address space
init_kmap:
	dd (init_ktbl - KSPACE + 0x003)
	times ((KSPACE / 0x400000) - 1) dd 0 ; Fill until KSPACE
	dd (init_ktbl - KSPACE + 0x203)
	times (1022 - (KSPACE / 0x400000)) dd 0	; Fill remainder of map
	dd (init_kmap - KSPACE + 0x203)

init_ktbl:
%assign i 0
%rep 	1024
		dd (i << 12) | 3
%assign i i+1
%endrep

section .data

global gdt
gdt:
align 4096
	dd 0x00000000, 0x00000000
	dd 0x0000FFFF, 0x00CF9A00
	dd 0x0000FFFF, 0x00CF9200
	dd 0x0000FFFF, 0x00CFBA00
	dd 0x0000FFFF, 0x00CFB200
	dd 0x0000FFFF, 0x00CFFA00
	dd 0x0000FFFF, 0x00CFF200
	dd 0x00000000, 0x0000E900 ; This will become the TSS

gdt_ptr:
align 4
	dw 0x003F	; 47 bytes limit
	dd gdt 		; Points to *virtual* GDT


section .mboot

dw 0x4D0A, 0x2079, 0x616E, 0x656D, 0x6920, 0x2073, 0x7A4F, 0x6D79
dw 0x6E61, 0x6964, 0x7361, 0x202C, 0x696B, 0x676E, 0x6F20, 0x2066
dw 0x696B, 0x676E, 0x3A73, 0x4C0A, 0x6F6F, 0x206B, 0x6E6F, 0x6D20
dw 0x2079, 0x6F77, 0x6B72, 0x2C73, 0x7920, 0x2065, 0x694D, 0x6867
dw 0x7974, 0x202C, 0x6E61, 0x2064, 0x6964, 0x7073, 0x6961, 0x0A72

; Multiboot header
MultiBootHeader:
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .text

extern init
extern int_return
global start
start:
	cli
	mov ecx, init_kmap - KSPACE	; Physical address
	mov cr3, ecx	; Load address into CR3
	mov ecx, cr4
	mov edx, cr0
	or ecx, 0x00000010	; Set 4MB page flag
	or edx, 0x80000000	; Set paging flag
	mov cr4, ecx		; Return to CR4 (make 4MB pgs)
	mov cr0, edx		; Return to CR0 (start paging)
	jmp 0x08:.upper		; Jump to the higher half (in new code segment)

.upper:
	mov ecx, gdt_ptr	; Load (real) GDT pointer
	lgdt [ecx]			; Load new GDT

	mov ecx, 0x10		; Reload all kernel data segments
	mov ds, cx
	mov es, cx
	mov fs, cx
	mov gs, cx
	mov ss, cx

	; Clear unneeded lomem identity map
	mov edx, init_kmap
	xor ecx, ecx
	mov [edx], ecx

	mov esp, (kstack + KSTACKSIZE)	; Setup init stack
	mov ebp, (kstack + KSTACKSIZE)	; and base pointer

	push eax	; Push multiboot magic number for identification

	; Push *virtual* multiboot pointer
	add ebx, KSPACE
	push ebx

	call init
	mov esp, eax
	jmp int_return
