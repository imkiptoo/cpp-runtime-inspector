//! @file inspector/Trace.cpp
//! @brief Trace state management implementation.

#include "Trace.h"
#include "Heap.h"
#include "StlEncoders.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace inspector {

namespace {

//! Format a pointer address as hex string.
std::string formatAddress(const void* ptr) {
    std::ostringstream ss;
    ss << "0x" << std::hex << reinterpret_cast<uintptr_t>(ptr);
    return ss.str();
}

} // anonymous namespace

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

void TraceState::recordVarInit(const std::string& name, const void* addr,
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

void TraceState::recordVarUpdate(const std::string& name, const void* addr,
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
    // Check event limit (Tier 6 resource limits)
    if (m_eventLimitReached) {
        return;  // Stop recording once limit is reached
    }
    if (m_steps.size() >= m_maxEvents) {
        m_eventLimitReached = true;
        return;
    }

    TraceStep step;
    step.line = line;
    step.event = kind;
    step.funcName = funcName;

    // Re-encode each frame's locals from their current memory contents.
    // Without this, mutations that happen via method calls or
    // CXXOperatorCallExpr (e.g. std::variant::operator=, std::optional::reset)
    // would not show up because the visitor only instruments built-in
    // BinaryOperator / UnaryOperator writes.
    for (auto& frame : m_stack) {
        for (auto& [name, var] : frame.locals) {
            if (var.addr && var.type) {
                var.value = encodeValue(var.addr, var.type);
            }
        }
    }

    // Snapshot the stack
    step.stack = m_stack;

    // Snapshot the heap state
    HeapTracker& heap = HeapTracker::instance();
    for (const Allocation* alloc : heap.getLiveAllocations()) {
        HeapObject obj;
        obj.heapId = alloc->heap_id;
        obj.typeName = alloc->type ? (alloc->type->spelling ? alloc->type->spelling : "<type>") : "<untyped>";
        obj.isArray = alloc->is_array;
        obj.arrayCount = alloc->array_count;

        // Encode the heap object's value
        if (alloc->type) {
            if (alloc->is_array && alloc->type->kind != TypeKind::Array) {
                // Create a temporary array descriptor for encoding
                ArrayValue av;
                av.elementTypeName = obj.typeName;
                const char* base = static_cast<const char*>(alloc->base);
                for (size_t i = 0; i < alloc->array_count; ++i) {
                    const void* elemAddr = base + (i * alloc->type->size);
                    auto holder = std::make_shared<EncodedValueHolder>();
                    holder->type = alloc->type;
                    holder->value = encodeValue(elemAddr, alloc->type);
                    av.elements.push_back(holder);
                }
                obj.value = av;
            } else {
                obj.value = encodeValue(alloc->base, alloc->type);
            }
        } else {
            // Untyped allocation (from malloc) - just show as integer
            obj.value = static_cast<long long>(0);
        }

        step.heap[alloc->heap_id] = std::move(obj);
    }

    m_steps.push_back(std::move(step));
}

MemoryRegion TraceState::classifyAddress(const void* addr) const {
    // Check heap first using the interval tree
    if (HeapTracker::instance().resolve(addr).has_value()) {
        return MemoryRegion::Heap;
    }

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

    // TODO: Parse /proc/self/maps for global detection (Linux)

    return MemoryRegion::Unknown;
}

void TraceState::setStackBounds(const void* low, const void* high) {
    m_stackLow = low;
    m_stackHigh = high;
}

EncodedValue TraceState::encodePrimitive(const void* addr,
                                          const TypeDescriptor* type) {
    if (!type || !addr)
        return static_cast<long long>(0);

    switch (type->kind) {
    case TypeKind::Int:
        if (type->size == sizeof(char))
            return static_cast<long long>(*static_cast<const signed char*>(addr));
        else if (type->size == sizeof(short))
            return static_cast<long long>(*static_cast<const short*>(addr));
        else if (type->size == sizeof(int))
            return static_cast<long long>(*static_cast<const int*>(addr));
        else if (type->size == sizeof(long))
            return static_cast<long long>(*static_cast<const long*>(addr));
        else
            return static_cast<long long>(*static_cast<const long long*>(addr));

    case TypeKind::UInt:
        if (type->size == sizeof(unsigned char))
            return static_cast<unsigned long long>(
                *static_cast<const unsigned char*>(addr));
        else if (type->size == sizeof(unsigned short))
            return static_cast<unsigned long long>(
                *static_cast<const unsigned short*>(addr));
        else if (type->size == sizeof(unsigned int))
            return static_cast<unsigned long long>(
                *static_cast<const unsigned int*>(addr));
        else if (type->size == sizeof(unsigned long))
            return static_cast<unsigned long long>(
                *static_cast<const unsigned long*>(addr));
        else
            return static_cast<unsigned long long>(
                *static_cast<const unsigned long long*>(addr));

    case TypeKind::Float:
        if (type->size == sizeof(float))
            return static_cast<double>(*static_cast<const float*>(addr));
        else
            return *static_cast<const double*>(addr);

    case TypeKind::Bool:
        return *static_cast<const bool*>(addr);

    case TypeKind::Char:
        return *static_cast<const char*>(addr);

    case TypeKind::Pointer: {
        const void* ptr = *static_cast<const void* const*>(addr);
        return encodePointer(ptr, type);
    }

    default:
        return static_cast<long long>(0);
    }
}

EncodedValue TraceState::encodeStruct(const void* addr,
                                       const TypeDescriptor* type) {
    StructValue sv;
    sv.typeName = type->spelling ? type->spelling : "<struct>";

    const char* baseAddr = static_cast<const char*>(addr);

    // First, recursively add fields from base classes
    if (type->bases && type->base_count > 0) {
        for (size_t i = 0; i < type->base_count; ++i) {
            const BaseInfo& base = type->bases[i];
            if (!base.type)
                continue;

            const void* baseObjAddr = baseAddr + base.offset;
            addFieldsFromType(sv, baseObjAddr, base.type);
        }
    }

    // Then add direct fields
    addFieldsFromType(sv, addr, type, false /* skipBases */);

    return sv;
}

void TraceState::addFieldsFromType(StructValue& sv, const void* addr,
                                    const TypeDescriptor* type, bool includeBases) {
    const char* baseAddr = static_cast<const char*>(addr);

    // Recursively add base class fields first (if requested)
    if (includeBases && type->bases && type->base_count > 0) {
        for (size_t i = 0; i < type->base_count; ++i) {
            const BaseInfo& base = type->bases[i];
            if (!base.type)
                continue;

            const void* baseObjAddr = baseAddr + base.offset;
            addFieldsFromType(sv, baseObjAddr, base.type, true);
        }
    }

    // Add this type's direct fields
    if (!type->fields || type->field_count == 0) {
        return;
    }

    for (size_t i = 0; i < type->field_count; ++i) {
        const FieldInfo& field = type->fields[i];

        // Skip vptr if configured (plan says we show it, but make it optional)
        // if (field.is_vptr) continue;

        const void* fieldAddr = baseAddr + field.offset;
        std::string fieldName = field.name ? field.name : "<unnamed>";

        // Avoid duplicate field names from multiple inheritance paths
        if (sv.fields.find(fieldName) != sv.fields.end())
            continue;

        sv.fieldOrder.push_back(fieldName);

        auto holder = std::make_shared<EncodedValueHolder>();
        holder->type = field.type;
        holder->value = encodeValue(fieldAddr, field.type);
        sv.fields[fieldName] = holder;
    }
}

EncodedValue TraceState::encodeArray(const void* addr,
                                      const TypeDescriptor* type) {
    ArrayValue av;

    if (!type->element_type) {
        av.elementTypeName = "void";
        return av;
    }

    av.elementTypeName =
        type->element_type->spelling ? type->element_type->spelling : "element";

    if (type->element_count == 0) {
        // Dynamic array or unknown size - just return empty
        return av;
    }

    const char* baseAddr = static_cast<const char*>(addr);
    size_t elementSize = type->element_type->size;

    for (size_t i = 0; i < type->element_count; ++i) {
        const void* elemAddr = baseAddr + (i * elementSize);
        auto holder = std::make_shared<EncodedValueHolder>();
        holder->type = type->element_type;
        holder->value = encodeValue(elemAddr, type->element_type);
        av.elements.push_back(holder);
    }

    return av;
}

EncodedValue TraceState::encodeEnum(long long value,
                                     const TypeDescriptor* type) {
    EnumValue_ ev;
    ev.value = value;
    ev.hasName = false;

    if (type && type->enum_values) {
        const char* name = lookupEnumName(type, value);
        if (name) {
            ev.enumName = name;
            ev.hasName = true;
        }
    }

    return ev;
}

EncodedValue TraceState::encodeUnion(const void* addr,
                                      const TypeDescriptor* type) {
    UnionValue uv;
    uv.typeName = type->spelling ? type->spelling : "<union>";

    // Copy raw bytes
    if (type->size > 0) {
        uv.rawBytes.resize(type->size);
        std::memcpy(uv.rawBytes.data(), addr, type->size);
    }

    // If we have field info, interpret the first field
    if (type->fields && type->field_count > 0) {
        const FieldInfo& field = type->fields[0];
        uv.firstFieldName = field.name ? field.name : "<unnamed>";

        const void* fieldAddr =
            static_cast<const char*>(addr) + field.offset;
        auto holder = std::make_shared<EncodedValueHolder>();
        holder->type = field.type;
        holder->value = encodeValue(fieldAddr, field.type);
        uv.firstFieldValue = holder;
    }

    return uv;
}

EncodedValue TraceState::encodeValue(const void* addr,
                                      const TypeDescriptor* type) {
    if (!type)
        return static_cast<long long>(0);

    switch (type->kind) {
    case TypeKind::Int:
    case TypeKind::UInt:
    case TypeKind::Float:
    case TypeKind::Bool:
    case TypeKind::Char:
    case TypeKind::Pointer:
    case TypeKind::Reference:
        return encodePrimitive(addr, type);

    case TypeKind::Struct:
        // Struct kind covers user types AND STL containers (which the
        // plugin classifies as Struct). encodeValueAtAddress dispatches
        // to the right STL encoder when applicable; otherwise it falls
        // through to encodeStruct.
        return encodeValueAtAddress(addr, type);

    case TypeKind::Array:
        return encodeArray(addr, type);

    case TypeKind::Enum: {
        // Read the underlying value based on size
        long long value = 0;
        if (type->size == sizeof(char))
            value = *static_cast<const signed char*>(addr);
        else if (type->size == sizeof(short))
            value = *static_cast<const short*>(addr);
        else if (type->size == sizeof(int))
            value = *static_cast<const int*>(addr);
        else
            value = *static_cast<const long long*>(addr);
        return encodeEnum(value, type);
    }

    case TypeKind::Union:
        return encodeUnion(addr, type);

    default:
        return static_cast<long long>(0);
    }
}

EncodedValue TraceState::encodePointer(const void* ptr,
                                        const TypeDescriptor* type) {
    // Handle null pointer
    if (!ptr) {
        std::vector<std::string> result;
        result.push_back("C_ADDRESS");
        result.push_back("0x0");
        result.push_back(type && type->spelling ? type->spelling : "void*");
        result.push_back("null");
        return result;
    }

    // Try to resolve to heap allocation
    HeapTracker& heap = HeapTracker::instance();
    auto resolved = heap.resolve(ptr);

    if (resolved.has_value()) {
        const Allocation* alloc = resolved->allocation;
        HeapRef ref;
        ref.heapId = alloc->heap_id;
        ref.offset = resolved->offset;
        ref.isDangling = alloc->freed;
        return ref;
    }

    // Not on heap - return as raw address
    std::vector<std::string> result;
    result.push_back("C_ADDRESS");
    result.push_back(formatAddress(ptr));
    result.push_back(type && type->spelling ? type->spelling : "void*");
    result.push_back(regionToString(classifyAddress(ptr)));
    return result;
}

int TraceState::recordAlloc(void* ptr, size_t size, const TypeDescriptor* type,
                            bool isArray, size_t arrayCount) {
    return HeapTracker::instance().insert(ptr, size, type, isArray, arrayCount,
                                          getCurrentStep());
}

void TraceState::recordFree(void* ptr) {
    HeapTracker::instance().markFreed(ptr, getCurrentStep());
}

void TraceState::checkLeaks() {
    HeapTracker& heap = HeapTracker::instance();
    auto leaked = heap.getLeakedAllocations();

    for (const Allocation* alloc : leaked) {
        std::string typeName = alloc->type
            ? (alloc->type->spelling ? alloc->type->spelling : "<type>")
            : "<untyped>";
        m_leakedAllocations.push_back({alloc->heap_id, typeName});
    }
}

EncodedValue TraceState::encodeValueAtAddress(const void* addr,
                                               const TypeDescriptor* type) {
    if (!type) {
        return static_cast<long long>(0);
    }

    // Check if this is an STL container type
    if (type->spelling) {
        StlContainerKind stlKind = identifyStlContainer(type->spelling);

        switch (stlKind) {
        case StlContainerKind::Vector:
            return encodeStdVector(addr, type->element_type, *this);

        case StlContainerKind::String:
            return encodeStdString(addr);

        case StlContainerKind::Array:
            return encodeStdArray(addr, type->element_type,
                                  type->element_count, *this);

        case StlContainerKind::Pair:
            // For pair, we'd need first/second types - use struct encoding
            return encodeStruct(addr, type);

        case StlContainerKind::UniquePtr:
            return encodeStdUniquePtr(addr, type->element_type, *this);

        case StlContainerKind::SharedPtr:
            return encodeStdSharedPtr(addr, type->element_type, *this);

        case StlContainerKind::Optional:
            return encodeStdOptional(addr, type->element_type, *this);

        case StlContainerKind::Variant:
            return encodeStdVariant(addr, type->size, type->element_type, *this);

        case StlContainerKind::Function:
            return encodeStdFunction(addr);

        case StlContainerKind::Map:
        case StlContainerKind::Set:
            // Complex tree traversal - use placeholder for now
            return encodeStdMap(addr, nullptr, nullptr, *this);

        case StlContainerKind::None:
        default:
            // Not an STL container, use regular encoding
            break;
        }
    }

    // Direct dispatch on kind to avoid infinite recursion through
    // encodeValue (which delegates struct types back to us).
    switch (type->kind) {
    case TypeKind::Struct: return encodeStruct(addr, type);
    case TypeKind::Array:  return encodeArray(addr, type);
    case TypeKind::Union:  return encodeUnion(addr, type);
    default:               return encodeValue(addr, type);
    }
}

void TraceState::recordThrow(const std::string& funcName, int line) {
    emitStep(EventKind::Throw, funcName, line);
}

void TraceState::recordCatch(const std::string& funcName,
                              const std::string& typeName, int line) {
    (void)typeName;  // For now, we just record the event
    emitStep(EventKind::Catch, funcName, line);
}

} // namespace inspector
