//! @file inspector/StdinCapture.cpp
//! @brief Implementation of stdin capture.

#include "StdinCapture.h"

#include <atomic>
#include <cstring>

namespace inspector {

// --- CapturingInputBuf implementation ---

CapturingInputBuf::CapturingInputBuf(std::streambuf* original)
    : m_original(original), m_buffer(4096, '\0') {
    // Initialize with empty buffer
    setg(nullptr, nullptr, nullptr);
}

CapturingInputBuf::int_type CapturingInputBuf::underflow() {
    if (gptr() < egptr()) {
        // Still have data in buffer
        return traits_type::to_int_type(*gptr());
    }

    // Need to read more from original
    std::streamsize bytesRead = m_original->sgetn(&m_buffer[0], static_cast<std::streamsize>(m_buffer.size()));
    if (bytesRead <= 0) {
        return traits_type::eof();
    }

    // Capture the input
    std::string newInput(&m_buffer[0], static_cast<size_t>(bytesRead));
    m_captured += newInput;
    m_lastInput += newInput;

    // Set up buffer pointers
    setg(&m_buffer[0], &m_buffer[0], &m_buffer[0] + bytesRead);
    return traits_type::to_int_type(*gptr());
}

std::streamsize CapturingInputBuf::xsgetn(char* s, std::streamsize n) {
    std::streamsize count = 0;

    while (count < n) {
        // First consume from internal buffer
        if (gptr() < egptr()) {
            std::streamsize available = egptr() - gptr();
            std::streamsize toCopy = std::min(available, n - count);
            std::memcpy(s + count, gptr(), static_cast<size_t>(toCopy));
            gbump(static_cast<int>(toCopy));
            count += toCopy;
        } else {
            // Need to refill buffer
            if (underflow() == traits_type::eof()) {
                break;
            }
        }
    }

    return count;
}

// --- StdinCapture implementation ---

namespace {
    std::streambuf* g_originalBuf = nullptr;
    CapturingInputBuf* g_capturingBuf = nullptr;
    std::atomic<bool> g_installed{false};
}

void StdinCapture::install() {
    if (g_installed.exchange(true)) {
        return; // Already installed
    }

    g_originalBuf = std::cin.rdbuf();
    g_capturingBuf = new CapturingInputBuf(g_originalBuf);
    std::cin.rdbuf(g_capturingBuf);
}

void StdinCapture::uninstall() {
    if (!g_installed.exchange(false)) {
        return; // Not installed
    }

    if (g_originalBuf) {
        std::cin.rdbuf(g_originalBuf);
        g_originalBuf = nullptr;
    }

    delete g_capturingBuf;
    g_capturingBuf = nullptr;
}

const std::string& StdinCapture::getCaptured() {
    static std::string empty;
    return g_capturingBuf ? g_capturingBuf->getCaptured() : empty;
}

const std::string& StdinCapture::getLastInput() {
    static std::string empty;
    return g_capturingBuf ? g_capturingBuf->getLastInput() : empty;
}

void StdinCapture::clearLastInput() {
    if (g_capturingBuf) {
        g_capturingBuf->clearLastInput();
    }
}

bool StdinCapture::isInstalled() {
    return g_installed.load();
}

} // namespace inspector
