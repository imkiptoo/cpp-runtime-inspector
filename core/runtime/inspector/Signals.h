//! @file inspector/Signals.h
//! @brief Signal handlers for crash detection.
//!
//! This module installs handlers for common crash signals (SIGSEGV, SIGABRT,
//! SIGFPE, SIGBUS) to emit a final crash event before the process terminates.

#pragma once

namespace inspector {

//! Install crash signal handlers.
//! This should be called once at program startup (typically by __inspector_enter
//! for the first function).
void installCrashHandlers();

//! Check if crash handlers have been installed.
bool crashHandlersInstalled();

} // namespace inspector
