//! @file see/StringSafe.cpp
//! @brief Safe string reading implementation.

#include "StringSafe.h"

#include <csignal>
#include <csetjmp>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace inspector {

namespace {

// Thread-local jump buffer for SIGSEGV recovery
thread_local sigjmp_buf s_jumpBuf;
thread_local bool s_inSafeRead = false;

// SIGSEGV handler that longjmps back to safe point
void segvHandler(int sig) {
    (void)sig;
    if (s_inSafeRead) {
        siglongjmp(s_jumpBuf, 1);
    }
}

} // anonymous namespace

std::string safeReadString(const char* ptr, size_t maxLen) {
    if (!ptr || maxLen == 0) {
        return "";
    }

    // Quick check for obvious invalid pointers
    if (!isPointerReadable(ptr)) {
        return "";
    }

    // Install temporary SIGSEGV handler
    struct sigaction newSa{}, oldSa{};
    newSa.sa_handler = segvHandler;
    sigemptyset(&newSa.sa_mask);
    newSa.sa_flags = 0;

    if (sigaction(SIGSEGV, &newSa, &oldSa) != 0) {
        return ""; // Can't install handler, bail out
    }

    std::string result;
    s_inSafeRead = true;

    if (sigsetjmp(s_jumpBuf, 1) == 0) {
        // Normal path: try to read the string
        for (size_t i = 0; i < maxLen; ++i) {
            char c = ptr[i];
            if (c == '\0') {
                break;
            }
            result += c;
        }
    }
    // else: caught SIGSEGV, result stays empty or partial

    s_inSafeRead = false;

    // Restore original handler
    sigaction(SIGSEGV, &oldSa, nullptr);

    return result;
}

bool isPointerReadable(const void* ptr) {
    if (!ptr) {
        return false;
    }

    // Check for obviously invalid addresses
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

    // NULL page (first 4KB typically unmapped)
    if (addr < 0x1000) {
        return false;
    }

#if defined(__APPLE__) || defined(__linux__)
    // On macOS/Linux, high addresses beyond reasonable bounds
    // This is a heuristic based on typical virtual memory layouts
#if defined(__LP64__) || defined(_LP64)
    // 64-bit: check for obviously invalid high bits
    if (addr > 0x00007FFFFFFFFFFFULL) {
        return false;
    }
#endif

    // Use mincore or similar to check if page is mapped
    // For simplicity, we just do the basic checks above
    // The actual safety comes from the SIGSEGV handler
#endif

    return true;
}

} // namespace inspector
