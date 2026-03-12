//! @file see/Trace.cpp
//! @brief Trace state management implementation.

#include "Trace.h"

#include <algorithm>

namespace see {

const char* regionToString(MemoryRegion region) {
    switch (region) {
    case MemoryRegion::Stack:
        return "stack";
    case MemoryRegion::Heap:
        return "heap";
    case MemoryRegion::Global:
        return "global";
    case MemoryRegion::Unknown:
    default:
        return "unknown";
    }
}

TraceState& TraceState::instance() {
    // Use a heap-allocated instance that is never deleted to ensure
    // it survives past atexit handlers. This is intentional - we need
    // the trace state to be available when the atexit handler runs to
    // emit the final JSON output.
    static TraceState* state = new TraceState();
    return *state;
}

void TraceState::pushFrame(const std::string& funcName, int line) {
    Frame frame;
    frame.funcName = funcName;
    frame.frameId = nextFrameId();
    frame.isHighlighted = true;
    frame.isZombie = false;

    // Mark previous top as not highlighted
    if (!m_stack.empty()) {
        m_stack.back().isHighlighted = false;
    }

    m_stack.push_back(std::move(frame));
    emitStep(EventKind::Call, funcName, line);
}

void TraceState::popFrame(int line) {
    if (m_stack.empty())
        return;

    std::string funcName = m_stack.back().funcName;
    emitStep(EventKind::Return, funcName, line);
    m_stack.pop_back();

    // Highlight new top frame
    if (!m_stack.empty()) {
        m_stack.back().isHighlighted = true;
    }
}

void TraceState::recordVarInit(const std::string& name, void* addr,
                                const TypeDescriptor* type, EncodedValue value,
                                int line) {
    Frame* frame = currentFrame();
    if (!frame)
        return;

    // Add to ordered names if not already present
    auto it = std::find(frame->orderedLocalNames.begin(),
                        frame->orderedLocalNames.end(), name);
    if (it == frame->orderedLocalNames.end()) {
        frame->orderedLocalNames.push_back(name);
    }

    VarState var;
    var.name = name;
    var.addr = addr;
    var.value = std::move(value);
    var.type = type;
    frame->locals[name] = std::move(var);

    emitStep(EventKind::StepLine, frame->funcName, line);
}

void TraceState::recordVarUpdate(const std::string& name, void* addr,
                                  const TypeDescriptor* type,
                                  EncodedValue value) {
    Frame* frame = currentFrame();
    if (!frame)
        return;

    auto it = frame->locals.find(name);
    if (it != frame->locals.end()) {
        it->second.value = std::move(value);
    } else {
        // Variable not seen before (could be parameter or global)
        frame->orderedLocalNames.push_back(name);
        VarState var;
        var.name = name;
        var.addr = addr;
        var.value = std::move(value);
        var.type = type;
        frame->locals[name] = std::move(var);
    }
}

void TraceState::recordStep(int line) {
    if (m_stack.empty())
        return;
    emitStep(EventKind::StepLine, m_stack.back().funcName, line);
}

Frame* TraceState::currentFrame() {
    if (m_stack.empty())
        return nullptr;
    return &m_stack.back();
}

void TraceState::emitStep(EventKind kind, const std::string& funcName,
                           int line) {
    TraceStep step;
    step.line = line;
    step.event = kind;
    step.funcName = funcName;

    // Snapshot the stack
    step.stack = m_stack;

    m_steps.push_back(std::move(step));
}

MemoryRegion TraceState::classifyAddress(const void* addr) const {
    // Check if in stack bounds
    if (m_stackLow && m_stackHigh) {
        if (addr >= m_stackLow && addr <= m_stackHigh) {
            return MemoryRegion::Stack;
        }
    }

    // Heuristic: high addresses are typically stack on most platforms
    // This is a fallback when bounds aren't set
    uintptr_t ptr = reinterpret_cast<uintptr_t>(addr);
#if defined(__APPLE__) || defined(__linux__)
    // Stack typically in high memory on these platforms
    // Very rough heuristic
    if (ptr > 0x7f0000000000ULL) {
        return MemoryRegion::Stack;
    }
#endif

    // TODO: Implement heap detection with interval tree (Tier 3)
    // TODO: Parse /proc/self/maps for global detection (Linux)

    return MemoryRegion::Unknown;
}

void TraceState::setStackBounds(const void* low, const void* high) {
    m_stackLow = low;
    m_stackHigh = high;
}

} // namespace see
