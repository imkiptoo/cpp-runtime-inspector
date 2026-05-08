//! @file StdoutCapture.h
//! @brief Capture stdout/stderr output for trace visualization.

#pragma once

#include <iostream>
#include <streambuf>
#include <string>
#include <mutex>

namespace inspector {

//! Custom streambuf that captures output while forwarding to original.
//! Used to intercept std::cout and std::cerr output.
class CapturingStreambuf : public std::streambuf {
public:
    //! Construct capturing streambuf that wraps an existing streambuf.
    //! @param original The original streambuf to forward output to (can be nullptr)
    explicit CapturingStreambuf(std::streambuf* original);

    //! Get all captured output.
    const std::string& getCaptured() const { return m_captured; }

    //! Clear captured output.
    void clear() { m_captured.clear(); }

protected:
    //! Handle single character output.
    int_type overflow(int_type ch) override;

    //! Handle string output.
    std::streamsize xsputn(const char* s, std::streamsize n) override;

    //! Sync the stream (flush).
    int sync() override;

private:
    std::streambuf* m_original;
    std::string m_captured;
};

//! Global stdout capture state.
class StdoutCapture {
public:
    //! Install stdout/stderr capture. Call once at program start.
    static void install();

    //! Uninstall and restore original streams.
    static void uninstall();

    //! Append output to the captured buffer (used by C stdio hooks).
    static void append(const char* data, size_t len);

    //! Append a single character.
    static void append(char c);

    //! Get all captured output.
    static const std::string& getCaptured();

    //! Clear captured output.
    static void clear();

    //! Check if capture is installed.
    static bool isInstalled() { return s_installed; }

private:
    static bool s_installed;
    static CapturingStreambuf* s_coutCapture;
    static CapturingStreambuf* s_cerrCapture;
    static std::streambuf* s_originalCout;
    static std::streambuf* s_originalCerr;
    static std::string s_capturedOutput;
    static std::mutex s_mutex;
};

} // namespace inspector
