//! @file see/Trace.h
//! @brief Trace state management for the See++ runtime.

#pragma once

#include "TypeInfo.h"

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace see {

//! Encoded value representation for OPT format.
//!
//! Can be:
//! - A primitive value (int, double, bool, char)
//! - A pointer/reference encoded as ["C_ADDRESS", "0x...", "type", "region"]
//! - A string value
using EncodedValue = std::variant<
    long long,                   // Int types
    unsigned long long,          // UInt types
    double,                      // Float types
    bool,                        // Bool
    char,                        // Char
    std::vector<std::string>,    // Pointer/reference: ["C_ADDRESS", addr, type, region]
    std::string                  // String values
>;

//! Memory region classification for pointers.
enum class MemoryRegion {
    Stack,
    Heap,
    Global,
    Unknown
};

//! Convert MemoryRegion to string.
const char* regionToString(MemoryRegion region);

//! A single local variable's state.
struct VarState {
    std::string name;
    void* addr;
    EncodedValue value;
    const TypeDescriptor* type;
};

//! One stack frame in the simulated call stack.
struct Frame {
    std::string funcName;
    int frameId;
    bool isHighlighted;
    bool isZombie;

    //! Ordered variable names for consistent rendering.
    std::vector<std::string> orderedLocalNames;

    //! Variable states by name.
    std::unordered_map<std::string, VarState> locals;
};

//! OPT trace event types.
enum class EventKind {
    Call,
    Return,
    StepLine,
    // Variable events are rolled into step_line in OPT format
};

//! A single trace step in OPT format.
struct TraceStep {
    int line;
    EventKind event;
    std::string funcName;

    //! Stack snapshot at this step.
    std::vector<Frame> stack;

    //! Global variables.
    std::unordered_map<std::string, VarState> globals;
    std::vector<std::string> orderedGlobals;

    //! Heap objects (Tier 3).
    // std::unordered_map<int, HeapObject> heap;

    //! Captured stdout.
    std::string stdout_capture;
};

//! Process-wide trace state.
class TraceState {
public:
    //! Get the singleton instance.
    static TraceState& instance();

    //! Push a new frame onto the stack.
    void pushFrame(const std::string& funcName, int line);

    //! Pop the top frame from the stack.
    void popFrame(int line);

    //! Record a variable initialization in the current frame.
    void recordVarInit(const std::string& name, void* addr,
                       const TypeDescriptor* type, EncodedValue value, int line);

    //! Record a variable update in the current frame.
    void recordVarUpdate(const std::string& name, void* addr,
                         const TypeDescriptor* type, EncodedValue value);

    //! Record a step at a line.
    void recordStep(int line);

    //! Get the current stack.
    const std::vector<Frame>& getStack() const { return m_stack; }

    //! Get the current frame (top of stack).
    Frame* currentFrame();

    //! Get all recorded steps.
    const std::vector<TraceStep>& getSteps() const { return m_steps; }

    //! Set the original source code.
    void setSourceCode(const std::string& code) { m_sourceCode = code; }

    //! Get the original source code.
    const std::string& getSourceCode() const { return m_sourceCode; }

    //! Check if trace has been finalized.
    bool isFinalized() const { return m_finalized; }

    //! Mark trace as finalized.
    void finalize() { m_finalized = true; }

    //! Get next frame ID.
    int nextFrameId() { return m_nextFrameId++; }

    //! Classify a memory address.
    MemoryRegion classifyAddress(const void* addr) const;

    //! Set stack boundaries for region detection.
    void setStackBounds(const void* low, const void* high);

private:
    TraceState() = default;

    void emitStep(EventKind kind, const std::string& funcName, int line);

    std::vector<Frame> m_stack;
    std::vector<TraceStep> m_steps;
    std::string m_sourceCode;
    bool m_finalized = false;
    int m_nextFrameId = 0;

    // Stack bounds for region detection
    const void* m_stackLow = nullptr;
    const void* m_stackHigh = nullptr;
};

} // namespace see
