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

#ifndef __CONFIG_ADDRESS_H
#define __CONFIG_ADDRESS_H

#define PAGESZ 0x1000

/* general layout ***********************************************************/

#define KERNEL_ADDR_BASE 0xF0000000
#define KERNEL_ADDR_SIZE 0x10000000

#define SYSTEM_ADDR_BASE 0xC0000000
#define SYSTEM_ADDR_SIZE 0x30000000

#define USPACE_ADDR_BASE 0x00000000
#define USPACE_ADDR_SIZE 0xC0000000

/* kernel layout ************************************************************/

#define KERNEL_BOOT_SIZE 0x800000

/* system layout ************************************************************/

/* userspace layout *********************************************************/

#endif/*__CONFIG_ADDRESS_H*/
