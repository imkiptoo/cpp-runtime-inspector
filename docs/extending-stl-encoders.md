# Extending STL Encoders

This guide explains how to add support for additional STL containers or
update encoders for new library versions.

## Overview

STL containers are encoded at runtime by reading their internal memory
layout. This is inherently fragile and tied to specific library implementations.

The current implementation targets:
- **libstdc++** (GCC 11-13) on Linux
- **libc++** (Apple/LLVM) on macOS (limited support)

## Architecture

![STL Encoder Flow](./images/stl-encoder-flow.svg)

### Container Detection

Container types are identified by pattern matching on type names:

```cpp
// core/runtime/inspector/StlEncoders.cpp
StlContainerKind identifyStlContainer(const std::string& typeName) {
    if (typeName.find("std::vector") != std::string::npos ||
        typeName.find("std::__1::vector") != std::string::npos) {
        return StlContainerKind::Vector;
    }
    // ...
}
```

Note: `std::__1::` is the libc++ namespace.

### Encoder Functions

Each container has a dedicated encoder:

```cpp
EncodedValue encodeStdVector(const void* addr, const TypeDescriptor* elementType,
                             TraceState& state);
EncodedValue encodeStdString(const void* addr);
EncodedValue encodeStdUniquePtr(const void* addr, const TypeDescriptor* pointeeType,
                                TraceState& state);
```

### Integration

The plugin skips instrumentation of STL containers:

```cpp
// core/plugin/Visitor.cpp
if (m_typeEncoder.isStlContainer(type)) {
    return true;  // Skip - handled by runtime
}
```

The runtime encodes them using internal layout knowledge:

```cpp
// core/runtime/inspector/Trace.cpp
if (stlKind != StlContainerKind::None) {
    return encodeStlContainer(addr, type, stlKind);
}
```

## Adding a New Container

### Step 1: Add Container Kind

Update the enum in `core/runtime/inspector/StlEncoders.h`:

```cpp
enum class StlContainerKind {
    None,
    Vector,
    String,
    Array,
    Pair,
    Map,
    Set,
    UniquePtr,
    SharedPtr,
    Optional,
    Deque,        // New
    List,         // New
    ForwardList   // New
};
```

### Step 2: Add Detection

Update `identifyStlContainer()`:

```cpp
if (typeName.find("std::deque") != std::string::npos ||
    typeName.find("std::__1::deque") != std::string::npos) {
    return StlContainerKind::Deque;
}
```

### Step 3: Implement Encoder

Create the encoder function:

```cpp
EncodedValue encodeStdDeque(const void* addr, const TypeDescriptor* elementType,
                            TraceState& state) {
    ArrayValue av;
    av.elementTypeName = elementType ? elementType->spelling : "unknown";

    if (!addr) {
        return av;
    }

    // Read internal layout (libstdc++ specific)
    // std::deque has: _M_map, _M_map_size, _M_start, _M_finish
    // This is complex - deque uses a segmented array

    // ... implementation details ...

    return av;
}
```

### Step 4: Register Encoder

Update the dispatch in `core/runtime/inspector/Trace.cpp`:

```cpp
case StlContainerKind::Deque:
    return encodeStdDeque(addr, elementType, *this);
```

### Step 5: Add Tests

Create a golden test:

```
tests/golden/stl_deque/
├── input.cpp
└── expected.json
```

```cpp
// input.cpp
#include <deque>

int main() {
    std::deque<int> d;
    d.push_back(1);
    d.push_front(0);
    return 0;
}
```

## Understanding Internal Layouts

### Finding Layout Information

1. **Read library headers**: Look in `/usr/include/c++/VERSION/bits/`
2. **Use debugger**: `p *((std::vector<int>*)addr)` in GDB
3. **Check ABI documentation**: https://gcc.gnu.org/onlinedocs/libstdc++/

### Common Patterns

#### Pointer-based Containers (vector, string, deque)

Typically store: `begin`, `end`, `capacity` pointers.

```cpp
// std::vector layout (simplified)
struct vector_impl {
    T* _M_start;
    T* _M_finish;
    T* _M_end_of_storage;
};
```

#### Node-based Containers (list, map, set)

Use tree or linked structures:

```cpp
// std::map tree node (simplified)
struct tree_node {
    int _M_color;           // Red-black tree color
    tree_node* _M_parent;
    tree_node* _M_left;
    tree_node* _M_right;
    // Key-value pair follows...
};
```

#### Smart Pointers

```cpp
// std::unique_ptr (simplified)
struct unique_ptr_impl {
    T* _M_ptr;
    // Deleter follows (usually empty)
};

// std::shared_ptr (simplified)
struct shared_ptr_impl {
    T* _M_ptr;
    control_block* _M_refcount;
};
```

## Handling Library Differences

### libstdc++ vs libc++

The internal layouts differ between implementations:

| Container  | libstdc++ namespace | libc++ namespace    |
|------------|---------------------|---------------------|
| vector     | `std::vector`       | `std::__1::vector`  |
| string     | `std::basic_string` | `std::__1::basic_string` |
| unique_ptr | `std::unique_ptr`   | `std::__1::unique_ptr` |

### Version Differences

Even within a library, layouts may change:

```cpp
// Version check example
#if _GLIBCXX_RELEASE >= 12
    // libstdc++ 12+ layout
#else
    // Older layout
#endif
```

## Error Handling

Encoders should never crash. Use defensive coding:

```cpp
EncodedValue encodeStdVector(const void* addr, ...) {
    ArrayValue av;
    av.elementTypeName = "unknown";

    if (!addr) {
        return av;  // Safe default
    }

    try {
        // ... encoding logic ...
    } catch (...) {
        return av;  // Return empty on error
    }

    return av;
}
```

## Testing Strategies

### Golden Tests

Create expected output for specific scenarios:

```cpp
// Test empty container
std::vector<int> v1;

// Test single element
std::vector<int> v2 = {42};

// Test multiple elements
std::vector<int> v3 = {1, 2, 3, 4, 5};

// Test after mutations
std::vector<int> v4 = {1, 2};
v4.push_back(3);
v4.pop_back();
```

### Cross-Version Testing

Test against multiple library versions in CI:

```yaml
matrix:
  include:
    - os: ubuntu-22.04  # libstdc++ 11
    - os: ubuntu-24.04  # libstdc++ 13
```

## Limitations

### Unsupported Scenarios

1. **Custom allocators**: No type information available
2. **Debug mode containers**: Different layout with extra fields
3. **Hash containers** (`unordered_*`): Complex bucket structure
4. **Concurrent containers**: Thread-safety concerns

### Known Issues

1. **std::map/set traversal**: Tree traversal implemented but limited to libstdc++ x86_64 layout, 256-entry cap
2. **String SSO**: Different implementations across platforms
3. **Nested containers**: Deep recursion may hit limits

## Future Work

Potential improvements:

1. **DWARF-based encoding**: Use debug info for layout discovery
2. **Container adapters**: Support `stack`, `queue`, `priority_queue`
3. **Custom formatter protocol**: Allow user-defined encoding functions
4. **Runtime version detection**: Automatically select correct layout
