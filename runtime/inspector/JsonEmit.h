//! @file inspector/JsonEmit.h
//! @brief OPT format JSON emission for the C++ Runtime Inspector runtime.

#pragma once

#include "Trace.h"

#include <nlohmann/json.hpp>

namespace inspector {

//! Emits trace data in OPT (Online Python Tutor) format.
class JsonEmitter {
public:
    //! Default maximum output size in bytes (50MB).
    static constexpr size_t DEFAULT_MAX_OUTPUT_SIZE = 50 * 1024 * 1024;

    //! Convert trace state to OPT format JSON.
    static nlohmann::json toOPT(const TraceState& state);

    //! Convert a single trace step to JSON.
    static nlohmann::json stepToJson(const TraceStep& step);

    //! Convert a stack frame to JSON.
    static nlohmann::json frameToJson(const Frame& frame);

    //! Convert an encoded value to JSON.
    static nlohmann::json valueToJson(const EncodedValue& value,
                                       const TypeDescriptor* type);

    //! Convert a heap object to JSON.
    static nlohmann::json heapObjectToJson(const HeapObject& obj);

    //! Emit trace to stderr with optional size limit.
    //! @param state The trace state to emit.
    //! @param maxOutputSize Maximum output size in bytes (0 = unlimited).
    //! @return true if output was truncated, false otherwise.
    static bool emit(const TraceState& state,
                     size_t maxOutputSize = DEFAULT_MAX_OUTPUT_SIZE);

    //! Check if output would exceed size limit.
    //! @return Estimated output size in bytes.
    static size_t estimateOutputSize(const TraceState& state);
};

} // namespace inspector
