/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2009, 2014
 * Robert Lougher <rob@jamvm.org.uk>.
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
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <OS.h>

#include "../../jam.h"

/* Haiku: use the kernel kit for thread and system information. */

void *nativeStackBase() {
    thread_info info;

    if(get_thread_info(find_thread(NULL), &info) != B_OK)
        return NULL;

    /* The stack grows down, so its base is the high end. */
    return info.stack_end;
}

int nativeAvailableProcessors() {
    system_info info;

    if(get_system_info(&info) != B_OK)
        return 1;

    return info.cpu_count;
}

/* Return 0, so that JamVM uses the fixed default heap sizes.  JamVM
   maps the whole maximum heap with one mmap() at startup.  On Haiku,
   this mmap() reserves the full size against RAM and swap, even if the
   pages are never used.  A maximum heap from the physical memory
   (1 GB on most machines) would thus reserve 1 GB for each JamVM
   instance.  Use -Xms and -Xmx to change the heap sizes. */

long long nativePhysicalMemory() {
    return 0;
}

char *nativeLibError() {
    return dlerror();
}

char *nativeLibPath() {
    return getenv("LIBRARY_PATH");
}

void *nativeLibOpen(char *path) {
    return dlopen(path, RTLD_LAZY);
}

void nativeLibClose(void *handle) {
    dlclose(handle);
}

void *nativeLibSym(void *handle, char *symbol) {
    return dlsym(handle, symbol);
}

char *nativeLibMapName(char *name) {
   char *buff = sysMalloc(strlen(name) + sizeof("lib.so") + 1);

   sprintf(buff, "lib%s.so", name);
   return buff;
}
