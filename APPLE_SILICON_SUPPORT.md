# JamVM Apple Silicon (aarch64) Support

This document describes the changes made to add macOS Apple Silicon (aarch64) support to JamVM.

## Overview

JamVM already supported aarch64 on Linux and 32-bit ARM on macOS (darwin). This work adds support for 64-bit ARM (aarch64) on macOS, enabling JamVM to run natively on Apple Silicon Macs (M1, M2, M3, etc.).

## Files Modified

### 1. `configure.ac`

- Added `aarch64*-*-darwin*` host detection pattern (around line 50):
  ```
  aarch64*-*-darwin*) host_cpu=aarch64 host_os=darwin libdl_needed=no ;;
  ```
- Added `src/os/darwin/aarch64/Makefile` to `AC_CONFIG_FILES`

### 2. `src/os/darwin/Makefile.am`

- Added `aarch64` to `DIST_SUBDIRS`:
  ```makefile
  DIST_SUBDIRS = powerpc i386 arm aarch64
  ```

### 3. `src/arch/aarch64.h` (shared header)

#### Fixed COMPARE_AND_SWAP_64/32 Macros

The original inline assembly had issues with LLVM's assembler when literal values like `0` were passed as parameters. The fix adds explicit casts in the asm constraints to ensure proper register sizing:

```c
#define COMPARE_AND_SWAP_64(addr, old_val, new_val)             \
({                                                              \
    int result;                                                 \
    uintptr_t read_val;                                         \
    __asm__ __volatile__ (                                      \
        "1:     ldaxr %2, %1\n"                                 \
        "       cmp %2, %3\n"                                   \
        "       b.ne 2f\n"                                      \
        "       stlxr %w0, %4, %1\n"                            \
        "       cmp %w0, wzr\n"                                 \
        "       b.ne 1b\n"                                      \
        "2:     cset %w0, eq"                                   \
    : "=&r" (result), "+Q" (*addr), "=&r" (read_val)            \
    : "r" ((uintptr_t)(old_val)), "r" ((uintptr_t)(new_val))    \
    : "cc");                                                    \
    result;                                                     \
})
```

#### FLUSH_CACHE - No Changes Needed

The `FLUSH_CACHE` macro uses `dc cvau` and `ic ivau` instructions along with cache line size variables (`aarch64_data_cache_line_len`, etc.). This works on both Linux and macOS because:

- The cache maintenance instructions (`dc cvau`, `ic ivau`, `dsb`, `isb`) are permitted in userspace on both platforms
- The cache line size variables are properly initialized in each platform's `init.c`:
  - Linux: via `mrs ctr_el0` instruction
  - macOS: via `sysctlbyname("hw.cachelinesize")`

Using the inline instructions directly (rather than macOS system APIs like `sys_dcache_flush`) avoids system call overhead, which matters since `FLUSH_CACHE` is used frequently by the inline-threaded interpreter when generating/patching code.

## Files Created

### 4. `src/os/darwin/aarch64/Makefile.am`

Standard build configuration for the darwin/aarch64 platform:

```makefile
noinst_LTLIBRARIES = libnative.la
libnative_la_SOURCES = init.c dll_md.c callNative.S

AM_CPPFLAGS = -I$(top_builddir)/src -I$(top_srcdir)/src
AM_CCASFLAGS = -I$(top_builddir)/src
```

### 5. `src/os/darwin/aarch64/init.c`

Platform initialization that retrieves cache line sizes. On macOS, the `mrs ctr_el0` instruction (used on Linux) is not permitted in userspace, so we use `sysctlbyname()` instead:

```c
#include <sys/sysctl.h>
#include "arch/aarch64.h"

void initialisePlatform() {
    size_t line_size;
    size_t size = sizeof(line_size);

    if (sysctlbyname("hw.cachelinesize", &line_size, &size, NULL, 0) == 0) {
        aarch64_data_cache_line_len = line_size;
        aarch64_instruction_cache_line_len = line_size;
    } else {
        /* Fallback to common Apple Silicon cache line size */
        aarch64_data_cache_line_len = 128;
        aarch64_instruction_cache_line_len = 128;
    }

    aarch64_data_cache_line_mask = ~(aarch64_data_cache_line_len - 1);
    aarch64_instruction_cache_line_mask =
        ~(aarch64_instruction_cache_line_len - 1);
}
```

### 6. `src/os/darwin/aarch64/dll_md.c`

JNI native calling convention handling. This is identical to the Linux version since the aarch64 ABI is the same on both platforms.

### 7. `src/os/darwin/aarch64/callNative.S`

Assembly for calling native methods, adapted for Mach-O object format:

- Function symbols use underscore prefix (`_callJNIMethod` instead of `callJNIMethod`)
- Local labels use `L` prefix (e.g., `Lscan_sig` instead of `scan_sig`)
- No `.type` directive (ELF-specific, not used in Mach-O)
- Used `mov w9, w3; sub sp, sp, x9` for stack allocation (LLVM assembler requires explicit register width handling)

## Technical Notes

### Why the CAS Macro Fix is Safe for All Platforms

The changes to `COMPARE_AND_SWAP_64` and `COMPARE_AND_SWAP_32` are applied unconditionally (not just for macOS) because:

1. **Explicit type casts** - Adding `(uintptr_t)(old_val)` in the asm constraints ensures correct register sizing regardless of whether a literal like `0` or a variable is passed. This is correct and beneficial for both GCC and Clang.

2. **Newline-separated assembly syntax** - Using `\n` instead of `;` with backslash continuation works identically on both GAS (GNU Assembler, used on Linux) and LLVM's integrated assembler (used on macOS).

The original code worked on Linux GCC by luck - GCC/GAS was more forgiving about implicit type handling in inline assembly constraints. The fixed version is more explicit and portable.

### What Must Remain macOS-Specific

**init.c implementation** - On Linux, `mrs ctr_el0` can read the Cache Type Register to determine cache line sizes. On macOS, this instruction causes SIGILL (illegal instruction); `sysctlbyname("hw.cachelinesize")` must be used instead. This is the only platform-specific difference in the aarch64 implementation.

## Building

```bash
autoreconf -fi
./configure CFLAGS="-g -O0"  # -O0 recommended for clang compatibility
make
```

### Interpreter Configuration Options

All interpreter modes work on Apple Silicon:

| Option | Execution Engine |
|--------|-----------------|
| (default) | inline-threaded interpreter with stack-caching |
| `--disable-int-caching` | inline-threaded interpreter |
| `--disable-int-inlining` | direct-threaded interpreter with stack-caching |
| `--disable-int-direct` | threaded interpreter with stack-caching |
| `--disable-int-threading` | switch-based interpreter |

To switch between configurations:

```bash
make clean
./configure CFLAGS="-g -O0" --disable-int-inlining  # or other options
make
```

## Testing

```bash
./src/jamvm -Xbootclasspath:/path/to/classes.zip:/path/to/glibj.zip -cp /path/to/classes HelloWorld
```

Where:
- `classes.zip` is JamVM's bootstrap classes (built in `src/classlib/gnuclasspath/lib/`)
- `glibj.zip` is GNU Classpath's class library
