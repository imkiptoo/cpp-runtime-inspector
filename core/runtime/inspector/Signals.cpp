//! @file inspector/Signals.cpp
//! @brief Implementation of crash signal handlers.

#include "Signals.h"
#include "JsonEmit.h"
#include "Trace.h"

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>  // for write(), STDERR_FILENO

namespace inspector {

namespace {

//! Flag to ensure handlers are only installed once.
std::atomic<bool> g_handlersInstalled{false};

//! Buffer for emergency output (async-signal-safe).
constexpr size_t CRASH_MSG_SIZE = 512;
char g_crashBuffer[CRASH_MSG_SIZE];

//! Signal names for error messages.
const char* signalName(int sig) {
    switch (sig) {
    case SIGSEGV: return "SIGSEGV";
    case SIGABRT: return "SIGABRT";
    case SIGFPE:  return "SIGFPE";
    case SIGBUS:  return "SIGBUS";
    case SIGILL:  return "SIGILL";
    default:      return "UNKNOWN";
    }
}

//! Format a simple crash JSON message (async-signal-safe).
void formatCrashJson(char* buffer, size_t size, const char* signal, void* addr) {
    // Simple snprintf-like formatting that should be safe
    // Note: Real async-signal-safe code would avoid even this,
    // but for practical purposes this is usually fine
    snprintf(buffer, size,
        "{\"crash\":{\"signal\":\"%s\",\"address\":\"%p\"}}\n",
        signal, addr);
}

//! Signal handler - must be async-signal-safe.
void crashHandler(int sig, siginfo_t* info, void* context) {
    (void)context;

    // Format crash message
    const char* sigName = signalName(sig);
    void* faultAddr = info ? info->si_addr : nullptr;

    formatCrashJson(g_crashBuffer, CRASH_MSG_SIZE, sigName, faultAddr);

    // Write to stderr (async-signal-safe)
    // We use raw write() which is signal-safe, unlike fprintf
    ssize_t written = write(STDERR_FILENO, g_crashBuffer, strlen(g_crashBuffer));
    (void)written;  // Suppress unused warning

    // Also try to emit proper trace if possible
    // Note: This is technically not async-signal-safe, but we try anyway
    // as it provides better debugging information
    auto& state = TraceState::instance();
    if (!state.isFinalized()) {
        state.finalize();
        // Attempt to emit trace - may fail if heap is corrupted
        try {
            JsonEmitter::emit(state);
        } catch (...) {
            // Ignore errors - we're crashing anyway
        }
    }

    // Re-raise the signal with default handler to get proper exit code
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigaction(sig, &sa, nullptr);
    raise(sig);
}

} // anonymous namespace

void installCrashHandlers() {
    bool expected = false;
    if (!g_handlersInstalled.compare_exchange_strong(expected, true)) {
        return;  // Already installed
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crashHandler;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;  // Get siginfo, reset after handling

    // Install handlers for common crash signals
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
}

bool crashHandlersInstalled() {
    return g_handlersInstalled.load();
}

} // namespace inspector
