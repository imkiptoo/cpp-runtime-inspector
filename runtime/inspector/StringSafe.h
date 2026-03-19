//! @file see/StringSafe.h
//! @brief Safe string reading with SIGSEGV protection.

#pragma once

#include <string>

namespace inspector {

//! Safely read a C-string, returning empty string if memory access fails.
//!
//! Uses a temporary SIGSEGV handler to catch invalid memory access.
//! Falls back to empty string if the pointer is not readable.
//!
//! @param ptr     The string pointer to read
//! @param maxLen  Maximum bytes to read
//! @return The string contents, or empty if unreadable
std::string safeReadString(const char* ptr, size_t maxLen);

//! Check if a pointer is likely readable.
//!
//! Uses platform-specific methods to check pointer validity.
//! This is a heuristic and may have false positives/negatives.
bool isPointerReadable(const void* ptr);

} // namespace inspector
