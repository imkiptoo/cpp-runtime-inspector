//! @file inspector/Trace.h
//! @brief Trace state management for the C++ Runtime Inspector runtime.

#pragma once

#include "TypeInfo.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace inspector {

// Forward declarations
struct StructValue;
struct ArrayValue;
struct Allocation;

//! Encoded struct value: ordered map of field name -> value
struct StructValue {
    std::string typeName;
    std::vector<std::string> fieldOrder;  //!< Ordered field names
    std::map<std::string, std::shared_ptr<struct EncodedValueHolder>> fields;
};

//! Encoded array value
struct ArrayValue {
    std::string elementTypeName;
    std::vector<std::shared_ptr<struct EncodedValueHolder>> elements;
};

//! Encoded enum value
struct EnumValue_ {
    std::string enumName;      //!< Enumerator name (if known)
    long long value;           //!< Underlying value
    bool hasName;              //!< True if enumName is valid
};

//! Encoded union value (show raw bytes and possibly first interpretable field)
struct UnionValue {
    std::string typeName;
    std::vector<uint8_t> rawBytes;  //!< Raw byte representation
    std::string firstFieldName;     //!< First field's name (for display)
    std::shared_ptr<struct EncodedValueHolder> firstFieldValue;  //!< First field interpreted
};

//! Heap reference: ["REF", heap_id] or ["REF_OFFSET", heap_id, offset]
struct HeapRef {
    int heapId;                      //!< Heap allocation ID
    size_t offset;                   //!< Byte offset from base (0 for exact pointer)
    bool isDangling;                 //!< True if pointing to freed memory
};

//! Variant for all encoded value types.
//!
//! Can be:
//! - A primitive value (int, double, bool, char)
//! - A pointer/reference encoded as ["C_ADDRESS", "0x...", "type", "region"]
//! - A string value
//! - A struct value (Tier 2)
//! - An array value (Tier 2)
//! - An enum value (Tier 2)
//! - A union value (Tier 2)
using EncodedValueVariant = std::variant<
    long long,                   // Int types
    unsigned long long,          // UInt types
    double,                      // Float types
    bool,                        // Bool
    char,                        // Char
    std::vector<std::string>,    // Pointer/reference: ["C_ADDRESS", addr, type, region]
    std::string,                 // String values
    StructValue,                 // Struct/class values (Tier 2)
    ArrayValue,                  // Array values (Tier 2)
    EnumValue_,                  // Enum values (Tier 2)
    UnionValue,                  // Union values (Tier 2)
    HeapRef                      // Heap pointer: ["REF", heap_id] (Tier 3)
>;

//! Holder for recursive value types
struct EncodedValueHolder {
    EncodedValueVariant value;
    const TypeDescriptor* type = nullptr;
};

//! Convenience alias for the main encoded value type
using EncodedValue = EncodedValueVariant;

//! Heap object state snapshot
struct HeapObject {
    int heapId;                      //!< Heap allocation ID
    std::string typeName;            //!< Type name for display
    bool isArray;                    //!< True if array allocation
    size_t arrayCount;               //!< Number of elements (for arrays)
    EncodedValue value;              //!< Encoded value (primitive, struct, or array)
};

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

    //! Heap objects snapshot (Tier 3).
    std::map<int, HeapObject> heap;

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

    //! Encode a struct value by reading its fields.
    EncodedValue encodeStruct(const void* addr, const TypeDescriptor* type);

    //! Encode an array value by reading its elements.
    EncodedValue encodeArray(const void* addr, const TypeDescriptor* type);

    //! Encode an enum value with name lookup.
    EncodedValue encodeEnum(long long value, const TypeDescriptor* type);

    //! Encode a union value (shows raw bytes and first field).
    EncodedValue encodeUnion(const void* addr, const TypeDescriptor* type);

    //! Encode a primitive value from memory.
    EncodedValue encodePrimitive(const void* addr, const TypeDescriptor* type);

    //! Recursively encode any value based on its type descriptor.
    EncodedValue encodeValue(const void* addr, const TypeDescriptor* type);

    //! Record a heap allocation.
    //! @return The assigned heap ID.
    int recordAlloc(void* ptr, size_t size, const TypeDescriptor* type,
                    bool isArray, size_t arrayCount);

    //! Record a heap deallocation.
    void recordFree(void* ptr);

    //! Encode a pointer value, resolving heap references.
    EncodedValue encodePointer(const void* ptr, const TypeDescriptor* type);

    //! Get current step number.
    int getCurrentStep() const { return static_cast<int>(m_steps.size()); }

    //! Check for memory leaks and emit leak events.
    void checkLeaks();

    //! Get leaked allocations (for JSON output).
    const std::vector<std::pair<int, std::string>>& getLeakedAllocations() const {
        return m_leakedAllocations;
    }

private:
    TraceState() = default;

    void emitStep(EventKind kind, const std::string& funcName, int line);

    //! Helper to add fields from a type (including inherited fields).
    void addFieldsFromType(StructValue& sv, const void* addr,
                           const TypeDescriptor* type, bool includeBases = true);

    std::vector<Frame> m_stack;
    std::vector<TraceStep> m_steps;
    std::string m_sourceCode;
    bool m_finalized = false;
    int m_nextFrameId = 0;

    // Stack bounds for region detection
    const void* m_stackLow = nullptr;
    const void* m_stackHigh = nullptr;

    // Leaked allocations detected at exit (heap_id, type_name)
    std::vector<std::pair<int, std::string>> m_leakedAllocations;
};

} // namespace inspector
