/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007
 * Robert Lougher <rob@jamvm.org.uk>.
 * Copyright (C) 2020 Simon South <simon@simonsouth.net>.
 * Copyright (C) 2026 INGELABS S.L. <info@ingelabs.com>.
 *
 * This file is part of JamVM.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <sys/sysctl.h>
#include "jam.h"
#include "arch/aarch64.h"

/* Length in bytes of the smallest line in the host system's data cache */
unsigned char aarch64_data_cache_line_len;

/* Mask used to align a virtual address to a line in the data cache */
uintptr_t aarch64_data_cache_line_mask;

/* Length in bytes of the smallest line in the host system's instruction
   cache */
unsigned char aarch64_instruction_cache_line_len;

/* Mask used to align a virtual address to a line in the instruction cache */
uintptr_t aarch64_instruction_cache_line_mask;

void initialisePlatform() {
    size_t line_size;
    size_t size = sizeof(line_size);

    /* On macOS, reading ctr_el0 is not permitted from userspace.
       Use sysctl to get the cache line size instead. */
    if (sysctlbyname("hw.cachelinesize", &line_size, &size, NULL, 0) != 0) {
        perror("Couldn't determine cache line size");
        exitVM(1);
    }

    aarch64_data_cache_line_len = line_size;
    aarch64_instruction_cache_line_len = line_size;

    aarch64_data_cache_line_mask = ~(aarch64_data_cache_line_len - 1);
    aarch64_instruction_cache_line_mask =
        ~(aarch64_instruction_cache_line_len - 1);
}
