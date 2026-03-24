# C++ Runtime Inspector Trace Format

C++ Runtime Inspector generates execution traces in the OPT (Online Python Tutor)
format, enabling compatibility with existing visualization frontends.

## Overview

The trace output is a JSON object with these top-level keys:

```json
{
  "code": "<original source code>",
  "trace": [<array of trace steps>],
  "memory_leaks": [<optional array of leak info>],
  "truncated": false,
  "truncation_reason": null
}
```

## Trace Steps

Each step in the trace array represents a point in execution:

```json
{
  "line": 5,
  "event": "call",
  "func_name": "main",
  "stack_to_render": [...],
  "globals": {},
  "ordered_globals": [],
  "heap": {},
  "stdout": ""
}
```

### Event Types

| Event       | Description                                    |
|-------------|------------------------------------------------|
| `call`      | Function entry                                 |
| `return`    | Function exit                                  |
| `step_line` | Statement execution or variable change         |
| `exception` | Exception thrown (throw expression)            |
| `catch`     | Exception caught (catch block entry)           |

### Stack Frames

Each frame in `stack_to_render`:

```json
{
  "frame_id": 0,
  "func_name": "main",
  "encoded_locals": {
    "x": 42,
    "ptr": ["C_ADDRESS", "0x7ffd1234", "int*", "stack"]
  },
  "ordered_varnames": ["x", "ptr"],
  "is_highlighted": true,
  "is_zombie": false
}
```

## Value Encoding

### Primitives

| Type      | JSON Representation        |
|-----------|----------------------------|
| Integer   | `42`, `-17`                |
| Unsigned  | `42`                       |
| Float     | `3.14`, `-0.5`             |
| Boolean   | `true`, `false`            |
| Character | `"a"` (as single-char str) |
| String    | `"hello world"`            |

### Pointers and References

```json
["C_ADDRESS", "0xADDR", "type*", "region"]
```

Region values:
- `"stack"` - Stack-allocated memory
- `"heap"` - Dynamically allocated memory
- `"global"` - Global/static storage
- `"unknown"` - Cannot determine region

### Heap References (Tier 3)

When a pointer points to tracked heap memory:

```json
["REF", 1]                    // Pointer to heap object 1
["REF_OFFSET", 1, 16]         // Pointer to offset 16 in heap object 1
["DANGLING", 1]               // Pointer to freed heap object 1
```

### Structs and Classes

```json
["C_STRUCT", "MyStruct", {
  "field1": 42,
  "field2": "hello",
  "nested": ["C_STRUCT", "Inner", {...}]
}]
```

### Arrays

```json
["C_ARRAY", "int", [1, 2, 3, 4, 5]]
```

Multi-dimensional arrays are nested:

```json
["C_ARRAY", "int[3]", [
  ["C_ARRAY", "int", [1, 2, 3]],
  ["C_ARRAY", "int", [4, 5, 6]]
]]
```

### Enums

Encoded enums show the symbolic name when available:

```json
"RED"           // enum value with known name
42              // enum value without symbolic name
```

### Unions

```json
["C_UNION", "MyUnion", {
  "firstField": 42,
  "__raw": "2a000000"
}]
```

## Heap Objects

The `heap` field maps heap IDs to heap objects:

```json
"heap": {
  "1": ["HEAP_PRIMITIVE", "int", 42],
  "2": ["HEAP_ARRAY", "int", [1, 2, 3]],
  "3": ["HEAP_STRUCT", "Node", [
    ["value", 10],
    ["next", ["REF", 4]]
  ]]
}
```

### Heap Object Types

| Type           | Format                                   |
|----------------|------------------------------------------|
| Primitive      | `["HEAP_PRIMITIVE", typename, value]`    |
| Array          | `["HEAP_ARRAY", typename, [elements]]`   |
| Struct/Class   | `["HEAP_STRUCT", typename, [[f,v],...]]` |

## Memory Leaks

Detected at program exit:

```json
"memory_leaks": [
  ["leaked", 5, "int"],
  ["leaked", 6, "Node"]
]
```

## Truncation

When resource limits are hit:

```json
{
  "truncated": true,
  "truncation_reason": "event_count_exceeded",
  "original_event_count": 150000,
  "included_event_count": 100000,
  ...
}
```

Or for output size limits:

```json
{
  "truncated": true,
  "truncation_reason": "output_size_exceeded",
  ...
}
```

## STL Container Encoding

STL containers are encoded as their logical contents:

### std::vector

```json
["C_ARRAY", "int", [1, 2, 3, 4, 5]]
```

### std::string

```json
"hello world"
```

### std::unique_ptr / std::shared_ptr

```json
["REF", 5]        // Points to heap object 5
["C_ADDRESS", "0x0", "int*", "null"]  // nullptr
```

### std::pair

```json
["C_STRUCT", "std::pair", {
  "first": "key",
  "second": 42
}]
```

## Complete Example

Input program:

```cpp
#include <vector>

struct Node {
    int value;
    Node* next;
};

int main() {
    Node* head = new Node{10, nullptr};
    head->next = new Node{20, nullptr};

    std::vector<int> v = {1, 2, 3};

    delete head->next;
    delete head;
    return 0;
}
```

Produces trace with heap tracking:

```json
{
  "code": "...",
  "trace": [
    {
      "event": "call",
      "line": 8,
      "func_name": "main",
      "stack_to_render": [...],
      "heap": {}
    },
    {
      "event": "step_line",
      "line": 9,
      "func_name": "main",
      "stack_to_render": [{
        "encoded_locals": {
          "head": ["REF", 1]
        }
      }],
      "heap": {
        "1": ["HEAP_STRUCT", "Node", [
          ["value", 10],
          ["next", ["C_ADDRESS", "0x0", "Node*", "null"]]
        ]]
      }
    }
  ]
}
```

## Compatibility

This format is compatible with:
- Python Tutor visualization frontend
- C++ Runtime Inspector web interface
- Custom visualization tools implementing OPT format

## Reference

See the original OPT format specification:
https://github.com/pgbovine/OnlinePythonTutor/blob/master/v3/docs/opt-trace-format.md
