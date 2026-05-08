//! @file inspector/StdinCapture.h
//! @brief Capture stdin input for trace visualization.

#pragma once

#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

namespace inspector {

//! A streambuf that records all input read from cin.
//! Data is read from the original streambuf but also captured for trace output.
class CapturingInputBuf : public std::streambuf {
public:
    explicit CapturingInputBuf(std::streambuf* original);

    //! Get all captured input as a single string.
    const std::string& getCaptured() const { return m_captured; }

    //! Get the last input line (most recently read).
    const std::string& getLastInput() const { return m_lastInput; }

    //! Clear the last input (called after recording it in trace).
    void clearLastInput() { m_lastInput.clear(); }

protected:
    int_type underflow() override;
    std::streamsize xsgetn(char* s, std::streamsize n) override;

private:
    std::streambuf* m_original;
    std::string m_captured;
    std::string m_lastInput;
    std::vector<char> m_buffer;
};

//! Global stdin capture manager.
class StdinCapture {
public:
    //! Install the capturing streambuf on std::cin.
    static void install();

    //! Restore the original streambuf.
    static void uninstall();

    //! Get all captured input.
    static const std::string& getCaptured();

    //! Get the last input line (for trace events).
    static const std::string& getLastInput();

    //! Clear the last input.
    static void clearLastInput();

    //! Check if capture is installed.
    static bool isInstalled();
};

} // namespace inspector
