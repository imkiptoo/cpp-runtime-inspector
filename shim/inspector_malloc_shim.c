//! @file inspector_malloc_shim.c
//! @brief LD_PRELOAD/DYLD_INSERT_LIBRARIES shim to intercept malloc/free.
//!
//! This shim intercepts C-style memory allocation functions (malloc, calloc,
//! realloc, free, posix_memalign) to track heap allocations that don't go
//! through C++ new/delete.
//!
//! Usage:
//!   Linux:  LD_PRELOAD=libinspector_malloc_shim.so ./program
//!   macOS:  DYLD_INSERT_LIBRARIES=libinspector_malloc_shim.dylib ./program
//!
//! The shim records allocations with type=nullptr (unknown type). The runtime
//! can backfill type information when the pointer is assigned to a typed
//! variable.

// _GNU_SOURCE is needed on Linux for RTLD_NEXT
#ifndef __APPLE__
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Forward declarations for the runtime hooks
extern void __inspector_alloc_malloc(void* ptr, size_t size);
extern void __inspector_dealloc_malloc(void* ptr);

// Real function pointers (populated lazily)
static void* (*real_malloc)(size_t) = NULL;
static void  (*real_free)(void*) = NULL;
static void* (*real_calloc)(size_t, size_t) = NULL;
static void* (*real_realloc)(void*, size_t) = NULL;
static int   (*real_posix_memalign)(void**, size_t, size_t) = NULL;

// Thread-local reentrancy guard to prevent infinite recursion
// (dlsym and other functions we call may use malloc internally)
static __thread int in_hook = 0;

// Bootstrap buffer for allocations during dlsym initialization
// dlsym may call malloc before we have the real malloc pointer
#define BOOTSTRAP_SIZE 65536
static char bootstrap_buffer[BOOTSTRAP_SIZE];
static size_t bootstrap_offset = 0;

// Flag to indicate initialization complete
static int initialized = 0;

//! Allocate from bootstrap buffer during early initialization.
static void* bootstrap_alloc(size_t size) {
    // Align to 16 bytes
    size_t aligned_offset = (bootstrap_offset + 15) & ~15;
    if (aligned_offset + size > BOOTSTRAP_SIZE) {
        // Out of bootstrap space - this shouldn't happen in practice
        return NULL;
    }
    void* ptr = bootstrap_buffer + aligned_offset;
    bootstrap_offset = aligned_offset + size;
    return ptr;
}

//! Check if pointer is in bootstrap buffer.
static int is_bootstrap_ptr(void* ptr) {
    char* p = (char*)ptr;
    return p >= bootstrap_buffer && p < bootstrap_buffer + BOOTSTRAP_SIZE;
}

//! Initialize real function pointers.
static void init_real_functions(void) {
    if (initialized) return;

    in_hook = 1;  // Prevent recursion during dlsym

    real_malloc = dlsym(RTLD_NEXT, "malloc");
    real_free = dlsym(RTLD_NEXT, "free");
    real_calloc = dlsym(RTLD_NEXT, "calloc");
    real_realloc = dlsym(RTLD_NEXT, "realloc");
    real_posix_memalign = dlsym(RTLD_NEXT, "posix_memalign");

    initialized = 1;
    in_hook = 0;
}

// ---------------------------------------------------------------------------
// Interposed functions
// ---------------------------------------------------------------------------

void* malloc(size_t size) {
    // Early bootstrap phase
    if (!initialized || in_hook) {
        if (!initialized) {
            init_real_functions();
            if (in_hook) {
                return bootstrap_alloc(size);
            }
        } else {
            return real_malloc(size);
        }
    }

    in_hook = 1;
    void* ptr = real_malloc(size);
    if (ptr) {
        __inspector_alloc_malloc(ptr, size);
    }
    in_hook = 0;

    return ptr;
}

void free(void* ptr) {
    // Ignore NULL and bootstrap pointers
    if (!ptr || is_bootstrap_ptr(ptr)) {
        return;
    }

    if (!initialized || in_hook) {
        if (initialized) {
            real_free(ptr);
        }
        return;
    }

    in_hook = 1;
    __inspector_dealloc_malloc(ptr);
    real_free(ptr);
    in_hook = 0;
}

void* calloc(size_t nmemb, size_t size) {
    // Early bootstrap phase - dlsym often calls calloc
    if (!initialized || in_hook) {
        if (!initialized) {
            init_real_functions();
            if (in_hook) {
                void* ptr = bootstrap_alloc(nmemb * size);
                if (ptr) {
                    memset(ptr, 0, nmemb * size);
                }
                return ptr;
            }
        } else {
            return real_calloc(nmemb, size);
        }
    }

    in_hook = 1;
    void* ptr = real_calloc(nmemb, size);
    if (ptr) {
        __inspector_alloc_malloc(ptr, nmemb * size);
    }
    in_hook = 0;

    return ptr;
}

void* realloc(void* ptr, size_t size) {
    // Handle NULL realloc (equivalent to malloc)
    if (!ptr) {
        return malloc(size);
    }

    // Handle bootstrap pointers - can't realloc them
    if (is_bootstrap_ptr(ptr)) {
        void* new_ptr = malloc(size);
        if (new_ptr) {
            // Copy what we can (we don't know original size)
            memcpy(new_ptr, ptr, size);
        }
        return new_ptr;
    }

    // Handle zero size (equivalent to free)
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    if (!initialized || in_hook) {
        if (initialized) {
            return real_realloc(ptr, size);
        }
        return NULL;
    }

    in_hook = 1;
    // Record deallocation of old pointer
    __inspector_dealloc_malloc(ptr);

    void* new_ptr = real_realloc(ptr, size);
    if (new_ptr) {
        // Record allocation of new pointer
        __inspector_alloc_malloc(new_ptr, size);
    }
    in_hook = 0;

    return new_ptr;
}

int posix_memalign(void** memptr, size_t alignment, size_t size) {
    if (!initialized || in_hook) {
        if (!initialized) {
            init_real_functions();
        }
        if (initialized) {
            return real_posix_memalign(memptr, alignment, size);
        }
        return -1;
    }

    in_hook = 1;
    int result = real_posix_memalign(memptr, alignment, size);
    if (result == 0 && *memptr) {
        __inspector_alloc_malloc(*memptr, size);
    }
    in_hook = 0;

    return result;
}

// ---------------------------------------------------------------------------
// Notes on platform-specific behavior
// ---------------------------------------------------------------------------
//
// Linux:   Use LD_PRELOAD=libinspector_malloc_shim.so ./program
// macOS:   Use DYLD_INSERT_LIBRARIES=libinspector_malloc_shim.dylib ./program
//
// Both platforms use dlsym(RTLD_NEXT, ...) to find the real allocator
// functions. On macOS, this works with DYLD_INSERT_LIBRARIES because the
// shim is loaded before libc.
//
// Note: On macOS with System Integrity Protection (SIP) enabled,
// DYLD_INSERT_LIBRARIES only works for binaries in non-protected
// directories (not /System, /usr/bin, etc.). User-built binaries work fine.
