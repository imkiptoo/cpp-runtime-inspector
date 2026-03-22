//! @file inspector/JsonEmit.h
//! @brief OPT format JSON emission for the C++ Runtime Inspector runtime.

#pragma once

#include "Trace.h"

#include <nlohmann/json.hpp>

namespace inspector {

//! Emits trace data in OPT (Online Python Tutor) format.
class JsonEmitter {
public:
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

    //! Emit trace to stderr.
    static void emit(const TraceState& state);
};

} // namespace inspector
