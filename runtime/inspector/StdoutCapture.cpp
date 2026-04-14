//! @file StdoutCapture.cpp
//! @brief Implementation of stdout/stderr capture.

#include "StdoutCapture.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

namespace inspector {

// ============================================================================
// CapturingStreambuf implementation
// ============================================================================

CapturingStreambuf::CapturingStreambuf(std::streambuf* original)
    : m_original(original) {}

std::streambuf::int_type CapturingStreambuf::overflow(int_type ch) {
    if (ch != traits_type::eof()) {
        char c = static_cast<char>(ch);
        m_captured += c;
        StdoutCapture::append(c);

        if (m_original) {
            return m_original->sputc(c);
        }
    }
    return ch;
}

std::streamsize CapturingStreambuf::xsputn(const char* s, std::streamsize n) {
    m_captured.append(s, static_cast<size_t>(n));
    StdoutCapture::append(s, static_cast<size_t>(n));

    if (m_original) {
        return m_original->sputn(s, n);
    }
    return n;
}

int CapturingStreambuf::sync() {
    if (m_original) {
        return m_original->pubsync();
    }
    return 0;
}

// ============================================================================
// StdoutCapture static members
// ============================================================================

bool StdoutCapture::s_installed = false;
CapturingStreambuf* StdoutCapture::s_coutCapture = nullptr;
CapturingStreambuf* StdoutCapture::s_cerrCapture = nullptr;
std::streambuf* StdoutCapture::s_originalCout = nullptr;
std::streambuf* StdoutCapture::s_originalCerr = nullptr;
std::string StdoutCapture::s_capturedOutput;
std::mutex StdoutCapture::s_mutex;

void StdoutCapture::install() {
    if (s_installed) return;

    std::lock_guard<std::mutex> lock(s_mutex);

    // Capture cout
    s_originalCout = std::cout.rdbuf();
    s_coutCapture = new CapturingStreambuf(s_originalCout);
    std::cout.rdbuf(s_coutCapture);

    // Note: We don't capture cerr because the trace JSON goes to stderr
    // If we captured cerr, we'd create infinite recursion

    s_installed = true;
}

void StdoutCapture::uninstall() {
    if (!s_installed) return;

    std::lock_guard<std::mutex> lock(s_mutex);

    // Restore cout
    if (s_originalCout) {
        std::cout.rdbuf(s_originalCout);
    }

    delete s_coutCapture;
    s_coutCapture = nullptr;
    s_originalCout = nullptr;

    s_installed = false;
}

void StdoutCapture::append(const char* data, size_t len) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_capturedOutput.append(data, len);
}

void StdoutCapture::append(char c) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_capturedOutput += c;
}

const std::string& StdoutCapture::getCaptured() {
    // No lock needed for read in single-threaded context
    return s_capturedOutput;
}

void StdoutCapture::clear() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_capturedOutput.clear();
}

} // namespace inspector

// ============================================================================
// C stdio function interception
// ============================================================================

// We need to intercept printf, puts, fputs, etc. that write to stdout.
// Using weak symbols allows us to override without breaking the original.

// Store pointer to real functions (resolved at runtime)
static FILE* (*real_stdout_ptr)() = nullptr;

// Helper to check if FILE* is stdout
static inline bool isStdout(FILE* stream) {
    return stream == stdout;
}

// Capture helper
static void captureToInspector(const char* data, size_t len) {
    if (inspector::StdoutCapture::isInstalled()) {
        inspector::StdoutCapture::append(data, len);
    }
}

// We use a different approach: wrap the output after it happens
// by providing inspector-specific printf variants that the instrumented
// code can call, or by using constructor-based initialization.

// For now, we rely on the C++ stream capture which handles most cases.
// The printf family can be added via LD_PRELOAD if needed, but for
// educational C++ code, cout/cerr coverage is usually sufficient.

// Alternative: Use compile-time instrumentation in the plugin to wrap
// printf calls. This is cleaner than runtime interposition.
