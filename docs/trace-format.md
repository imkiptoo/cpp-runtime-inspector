# C++ Runtime Inspector Trace Format

C++ Runtime Inspector generates execution traces in the OPT (Online Python Tutor) format, enabling compatibility with existing visualization frontends.

## Overview

The trace output is a JSON object with two top-level keys:

```json
{
  "code": "<original source code>",
  "trace": [<array of trace steps>]
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

| Event | Description |
|-------|-------------|
| `call` | Function entry |
| `return` | Function exit |
| `step_line` | Statement execution or variable change |

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

### Value Encoding

| Type | JSON Representation |
|------|---------------------|
| Integer | `42`, `-17` |
| Float | `3.14`, `-0.5` |
| Boolean | `true`, `false` |
| Character | `"a"` (as string) |
| Pointer | `["C_ADDRESS", "0xADDR", "type*", "region"]` |
| String | `"hello world"` |

### Memory Regions

Pointers include a region hint:
- `"stack"` - Stack-allocated
- `"heap"` - Heap-allocated (Tier 3)
- `"global"` - Global/static storage
- `"unknown"` - Cannot determine

## Example

Input program:

```cpp
int main() {
    int a = 5;
    int b = a + 3;
    return 0;
}
```

Produces trace:

```json
{
  "code": "",
  "trace": [
    {
      "event": "call",
      "line": 1,
      "func_name": "main",
      "stack_to_render": [{
        "frame_id": 0,
        "func_name": "main",
        "encoded_locals": {},
        "ordered_varnames": [],
        "is_highlighted": true,
        "is_zombie": false
      }],
      "globals": {},
      "ordered_globals": [],
      "heap": {},
      "stdout": ""
    },
    {
      "event": "step_line",
      "line": 0,
      "func_name": "main",
      "stack_to_render": [{
        "frame_id": 0,
        "func_name": "main",
        "encoded_locals": {"a": 5},
        "ordered_varnames": ["a"],
        "is_highlighted": true,
        "is_zombie": false
      }],
      "globals": {},
      "ordered_globals": [],
      "heap": {},
      "stdout": ""
    },
    {
      "event": "step_line",
      "line": 0,
      "func_name": "main",
      "stack_to_render": [{
        "frame_id": 0,
        "func_name": "main",
        "encoded_locals": {"a": 5, "b": 8},
        "ordered_varnames": ["a", "b"],
        "is_highlighted": true,
        "is_zombie": false
      }],
      "globals": {},
      "ordered_globals": [],
      "heap": {},
      "stdout": ""
    },
    {
      "event": "return",
      "line": 4,
      "func_name": "main",
      "stack_to_render": [{
        "frame_id": 0,
        "func_name": "main",
        "encoded_locals": {"a": 5, "b": 8},
        "ordered_varnames": ["a", "b"],
        "is_highlighted": true,
        "is_zombie": false
      }],
      "globals": {},
      "ordered_globals": [],
      "heap": {},
      "stdout": ""
    }
  ]
}
```

## Compatibility

This format is compatible with:
- Python Tutor visualization frontend
- C++ Runtime Inspector web interface
- Custom visualization tools implementing OPT format

## Future Extensions

Tier 2/3 features will add:
- `heap` object containing heap allocations
- Struct/class field expansion in `encoded_locals`
- Array element tracking
- Global variable tracking in `globals`
