# Frontend Integration Guide

This guide explains how to consume C++ Runtime Inspector traces in
visualization frontends.

## Overview

C++ Runtime Inspector produces traces in the OPT (Online Python Tutor) format,
making it compatible with existing visualization tools.

## Integration Options

### 1. Direct Command-Line

```bash
# Instrument, compile, and run
clang++ -fsyntax-only -fplugin=libInspectorPlugin.so input.cpp
clang++ input.cpp.instrumented.cpp libinspector_runtime.a -o program
./program 2> trace.json

# Parse the trace
cat trace.json | jq '.trace | length'  # Number of steps
```

### 2. API Server HTTP Interface

The API server provides a simple HTTP interface:

```bash
# Start the server
python services/api/server.py --port 8080

# Submit code and get trace
curl -X POST http://localhost:8080/trace \
     -H "Content-Type: text/plain" \
     -d 'int main() { int x = 5; return 0; }' | jq
```

See [services/api/README.md](../services/api/README.md) for details.

### 3. Docker Container

For isolated execution:

```bash
# Build the sandbox
docker build -t cpp-inspector-sandbox -f services/sandbox/Dockerfile .

# Submit code
echo 'int main() { int x = 5; return 0; }' | \
    docker run --rm -i cpp-inspector-sandbox > trace.json
```

## Trace Format

See [trace-format.md](trace-format.md) for the complete specification.

### Key Fields

```json
{
  "code": "int main() { ... }",
  "trace": [
    {
      "line": 1,
      "event": "call",
      "func_name": "main",
      "stack_to_render": [...],
      "heap": {...}
    }
  ]
}
```

### Parsing Examples

#### JavaScript

```javascript
const trace = JSON.parse(traceJson);

// Iterate through steps
for (const step of trace.trace) {
    console.log(`Line ${step.line}: ${step.event} in ${step.func_name}`);

    // Get current frame
    const currentFrame = step.stack_to_render[step.stack_to_render.length - 1];
    console.log('Variables:', currentFrame.encoded_locals);
}

// Check for memory leaks
if (trace.memory_leaks?.length > 0) {
    console.log('Memory leaks detected:', trace.memory_leaks);
}
```

#### Python

```python
import json

with open('trace.json') as f:
    trace = json.load(f)

for step in trace['trace']:
    print(f"Line {step['line']}: {step['event']} in {step['func_name']}")

    # Access heap objects
    for heap_id, obj in step['heap'].items():
        obj_type = obj[0]  # HEAP_PRIMITIVE, HEAP_ARRAY, or HEAP_STRUCT
        obj_typename = obj[1]
        obj_value = obj[2]
        print(f"  Heap {heap_id}: {obj_typename} = {obj_value}")
```

## Value Decoding

### Primitives

```javascript
function decodeValue(encoded) {
    if (typeof encoded === 'number') return encoded;
    if (typeof encoded === 'boolean') return encoded;
    if (typeof encoded === 'string') return encoded;
    if (!Array.isArray(encoded)) return encoded;

    const [tag, ...rest] = encoded;

    switch (tag) {
        case 'C_ADDRESS':
            return { type: 'pointer', address: rest[0], typename: rest[1], region: rest[2] };
        case 'REF':
            return { type: 'heap_ref', heapId: rest[0] };
        case 'REF_OFFSET':
            return { type: 'heap_ref', heapId: rest[0], offset: rest[1] };
        case 'DANGLING':
            return { type: 'dangling', heapId: rest[0] };
        case 'C_STRUCT':
            return { type: 'struct', typename: rest[0], fields: rest[1] };
        case 'C_ARRAY':
            return { type: 'array', elementType: rest[0], elements: rest[1] };
        case 'C_UNION':
            return { type: 'union', typename: rest[0], value: rest[1] };
        default:
            return { type: 'unknown', raw: encoded };
    }
}
```

### Building Pointer Graphs

```javascript
function buildPointerGraph(step) {
    const nodes = new Map();
    const edges = [];

    // Add stack variables
    for (const frame of step.stack_to_render) {
        for (const [name, value] of Object.entries(frame.encoded_locals)) {
            const nodeId = `stack:${frame.frame_id}:${name}`;
            nodes.set(nodeId, { type: 'stack', name, value });

            // Check for heap references
            if (Array.isArray(value) && value[0] === 'REF') {
                edges.push({ from: nodeId, to: `heap:${value[1]}` });
            }
        }
    }

    // Add heap objects
    for (const [heapId, obj] of Object.entries(step.heap)) {
        const nodeId = `heap:${heapId}`;
        nodes.set(nodeId, { type: 'heap', heapId, obj });

        // Find pointer fields in struct
        if (obj[0] === 'HEAP_STRUCT') {
            for (const [fieldName, fieldValue] of obj[2]) {
                if (Array.isArray(fieldValue) && fieldValue[0] === 'REF') {
                    edges.push({
                        from: nodeId,
                        to: `heap:${fieldValue[1]}`,
                        field: fieldName
                    });
                }
            }
        }
    }

    return { nodes, edges };
}
```

## Visualization Strategies

### Step-by-Step Playback

```javascript
class TracePlayer {
    constructor(trace) {
        this.trace = trace;
        this.currentStep = 0;
    }

    get step() {
        return this.trace.trace[this.currentStep];
    }

    next() {
        if (this.currentStep < this.trace.trace.length - 1) {
            this.currentStep++;
            return true;
        }
        return false;
    }

    prev() {
        if (this.currentStep > 0) {
            this.currentStep--;
            return true;
        }
        return false;
    }

    seekToLine(line) {
        for (let i = 0; i < this.trace.trace.length; i++) {
            if (this.trace.trace[i].line === line) {
                this.currentStep = i;
                return true;
            }
        }
        return false;
    }
}
```

### Diff-Based Updates

For efficient rendering, compute diffs between steps:

```javascript
function diffSteps(prev, curr) {
    const changes = [];

    // Compare stack frames
    const prevFrameIds = new Set(prev.stack_to_render.map(f => f.frame_id));
    const currFrameIds = new Set(curr.stack_to_render.map(f => f.frame_id));

    // New frames
    for (const frame of curr.stack_to_render) {
        if (!prevFrameIds.has(frame.frame_id)) {
            changes.push({ type: 'frame_added', frame });
        }
    }

    // Removed frames
    for (const frame of prev.stack_to_render) {
        if (!currFrameIds.has(frame.frame_id)) {
            changes.push({ type: 'frame_removed', frame });
        }
    }

    // Changed variables
    for (const currFrame of curr.stack_to_render) {
        const prevFrame = prev.stack_to_render.find(f => f.frame_id === currFrame.frame_id);
        if (prevFrame) {
            for (const [name, value] of Object.entries(currFrame.encoded_locals)) {
                const prevValue = prevFrame.encoded_locals[name];
                if (JSON.stringify(value) !== JSON.stringify(prevValue)) {
                    changes.push({
                        type: prevValue === undefined ? 'var_added' : 'var_changed',
                        frame: currFrame.frame_id,
                        name,
                        oldValue: prevValue,
                        newValue: value
                    });
                }
            }
        }
    }

    // Heap changes
    for (const [heapId, obj] of Object.entries(curr.heap)) {
        if (!prev.heap[heapId]) {
            changes.push({ type: 'heap_alloc', heapId, obj });
        } else if (JSON.stringify(obj) !== JSON.stringify(prev.heap[heapId])) {
            changes.push({ type: 'heap_changed', heapId, obj });
        }
    }

    for (const heapId of Object.keys(prev.heap)) {
        if (!curr.heap[heapId]) {
            changes.push({ type: 'heap_freed', heapId });
        }
    }

    return changes;
}
```

## Error Handling

### Truncated Traces

```javascript
if (trace.truncated) {
    console.warn(`Trace truncated: ${trace.truncation_reason}`);
    console.log(`Showing ${trace.included_event_count} of ${trace.original_event_count} events`);
}
```

### Memory Leaks

```javascript
if (trace.memory_leaks?.length > 0) {
    for (const leak of trace.memory_leaks) {
        const [, heapId, typeName] = leak;
        console.error(`Memory leak: heap object ${heapId} (${typeName}) was not freed`);
    }
}
```

### Crash Indicators

Check the last event for crash information:

```javascript
const lastStep = trace.trace[trace.trace.length - 1];
// If trace ends abruptly, check stderr for crash JSON
```

## React Component Example

```jsx
function TraceViewer({ trace }) {
    const [currentStep, setCurrentStep] = useState(0);
    const step = trace.trace[currentStep];

    return (
        <div className="trace-viewer">
            <div className="controls">
                <button onClick={() => setCurrentStep(s => Math.max(0, s - 1))}>
                    Prev
                </button>
                <span>Step {currentStep + 1} / {trace.trace.length}</span>
                <button onClick={() => setCurrentStep(s => Math.min(trace.trace.length - 1, s + 1))}>
                    Next
                </button>
            </div>

            <div className="source-code">
                <SourceHighlighter code={trace.code} currentLine={step.line} />
            </div>

            <div className="stack">
                {step.stack_to_render.map(frame => (
                    <StackFrame key={frame.frame_id} frame={frame} />
                ))}
            </div>

            <div className="heap">
                {Object.entries(step.heap).map(([id, obj]) => (
                    <HeapObject key={id} id={id} object={obj} />
                ))}
            </div>
        </div>
    );
}
```

## Compatibility Notes

### Python Tutor Compatibility

The trace format is designed to be compatible with Python Tutor's visualization
engine. You can use existing Python Tutor frontends with minor modifications.

### Differences from Python

1. **Heap references**: C++ uses explicit pointer arrows, Python uses implicit references
2. **Memory regions**: C++ distinguishes stack/heap/global, Python doesn't
3. **Type information**: C++ includes precise type names
4. **Memory leaks**: Only relevant for C++ (manual memory management)

## Future Enhancements

Planned API improvements:

1. **WebSocket streaming**: Real-time trace updates during execution
2. **Breakpoint support**: Pause at specific lines
3. **Variable watches**: Subscribe to specific variable changes
4. **Batch processing**: Trace multiple files in one request
