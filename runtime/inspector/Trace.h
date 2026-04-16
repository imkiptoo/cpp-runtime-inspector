//! @file inspector/Trace.h
//! @brief Trace state management for the C++ Runtime Inspector runtime.

#pragma once

#include "TypeInfo.h"

#include <map>
#include <memory>
#include <optional>
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
    std::string dynamicType;              //!< Dynamic (most-derived) class name; empty if non-polymorphic or unknown
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
    size_t sizeBytes;                //!< Size of allocation in bytes
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
    const void* addr;
    EncodedValue value;
    const TypeDescriptor* type;
    size_t sizeBytes = 0;            //!< Size of variable in bytes
};

//! One stack frame in the simulated call stack.
struct Frame {
    std::string funcName;
    int frameId;
    bool isHighlighted;
    bool isZombie;
    bool isGhostDtor = false;  //!< True if this is a synthetic frame for temporary destruction

    //! Ordered variable names for consistent rendering.
    std::vector<std::string> orderedLocalNames;

    //! Variable states by name.
    std::unordered_map<std::string, VarState> locals;

    //! Total size of local variables in this frame in bytes.
    size_t stackSizeBytes = 0;
};

//! OPT trace event types.
enum class EventKind {
    Call,
    Return,
    StepLine,
    Throw,     //!< Exception thrown (Tier 5)
    Catch,     //!< Exception caught (Tier 5)
    // Variable events are rolled into step_line in OPT format
};

//! Lifecycle kind for Rule-of-5 special member functions.
enum class LifecycleKind : uint8_t {
    None = 0,
    DefaultCtor,
    CopyCtor,
    MoveCtor,
    CopyAssign,
    MoveAssign,
    Dtor,
};

//! Convert LifecycleKind to string representation.
const char* lifecycleKindToString(LifecycleKind kind);

//! A single trace step in OPT format.
struct TraceStep {
    int line;
    EventKind event;
    std::string funcName;

    //! Lifecycle kind for special member functions (Rule-of-5).
    //! Only set for constructor/destructor/assignment operator calls.
    LifecycleKind lifecycle = LifecycleKind::None;

    //! Stack snapshot at this step.
    std::vector<Frame> stack;

    //! Global variables.
    std::unordered_map<std::string, VarState> globals;
    std::vector<std::string> orderedGlobals;

    //! Heap objects snapshot (Tier 3).
    std::map<int, HeapObject> heap;

    //! Captured stdout.
    std::string stdout_capture;

    //! Captured stdin (input read since last step).
    std::string stdin_input;

    //! Return value (for return events).
    std::optional<EncodedValue> return_value;

    //! Total stack memory (all frames) in bytes.
    size_t stackTotalBytes = 0;

    //! Total heap memory (all live allocations) in bytes.
    size_t heapTotalBytes = 0;
};

//! Process-wide trace state.
class TraceState {
public:
    //! Get the singleton instance.
    static TraceState& instance();

    //! Push a new frame onto the stack.
    void pushFrame(const std::string& funcName, int line);

    //! Push a new frame with lifecycle annotation (for Rule-of-5 tracking).
    void pushFrameWithLifecycle(const std::string& funcName, int line, LifecycleKind lifecycle);

    //! Record a ghost destructor frame for temporary object destruction.
    //! Creates an ephemeral frame that appears briefly to show the destruction event.
    void recordGhostDtor(const std::string& typeName, int line);

    //! Pop the top frame from the stack.
    void popFrame(int line);

    //! Record a variable initialization in the current frame.
    void recordVarInit(const std::string& name, const void* addr,
                       const TypeDescriptor* type, EncodedValue value, int line);

    //! Record a variable update in the current frame.
    void recordVarUpdate(const std::string& name, const void* addr,
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

    //! Encode a value at a given address (used by STL encoders).
    //! This is a public wrapper for encodeValue that handles STL detection.
    EncodedValue encodeValueAtAddress(const void* addr, const TypeDescriptor* type);

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

    //! Record a throw expression (Tier 5).
    void recordThrow(const std::string& funcName, int line);

    //! Record entering a catch block (Tier 5).
    void recordCatch(const std::string& funcName, const std::string& typeName, int line);

    //! Record a return value (called before popFrame).
    void recordReturnValue(EncodedValue value);

    //! Register a global/constexpr variable.
    void registerGlobal(const std::string& name, const TypeDescriptor* type,
                        EncodedValue value);

    //! Set the maximum number of trace events (Tier 6 resource limits).
    //! Default is 100000. When exceeded, recording stops.
    void setMaxEvents(size_t maxEvents) { m_maxEvents = maxEvents; }

    //! Check if event limit has been reached.
    bool isEventLimitReached() const { return m_eventLimitReached; }

private:
    TraceState() = default;

    void emitStep(EventKind kind, const std::string& funcName, int line,
                  LifecycleKind lifecycle = LifecycleKind::None);

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

    // Resource limits (Tier 6)
    size_t m_maxEvents = 100000;
    bool m_eventLimitReached = false;

    // Pending return value (set by recordReturnValue, consumed by next return event)
    std::optional<EncodedValue> m_pendingReturnValue;

    // Global/constexpr variables
    std::unordered_map<std::string, VarState> m_globals;
    std::vector<std::string> m_orderedGlobals;

    // Unique types encountered during trace (for type metadata output)
    std::unordered_map<std::string, const TypeDescriptor*> m_encounteredTypes;

public:
    //! Get the types encountered during trace (for type metadata output).
    const std::unordered_map<std::string, const TypeDescriptor*>& getEncounteredTypes() const {
        return m_encounteredTypes;
    }

    //! Register a type as encountered.
    void registerType(const TypeDescriptor* type);
};

} // namespace inspector
