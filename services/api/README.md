# C++ Runtime Inspector - Frontend Adapter

HTTP service that accepts C++ source code and returns OPT-format trace JSON.

## Overview

This is the v1 HTTP-only implementation. WebSocket streaming for real-time
trace updates is planned for future versions.

## Quick Start

### Local Development

```bash
# Build the inspector first
cmake -B cmake-build-debug && cmake --build cmake-build-debug

# Run the adapter
cd frontend-adapter
python3 server.py
```

### Using Docker

```bash
# Build with release config
cmake -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release

# Build adapter image
docker build -t cpp-inspector-adapter -f frontend-adapter/Dockerfile .

# Run
docker run -p 8080:8080 cpp-inspector-adapter
```

## API

### POST /trace

Submit C++ source code and receive an OPT-format trace.

**Request:**
- Content-Type: `text/plain` or `application/json`
- Body: C++ source code (raw text)

**Response:**
- Content-Type: `application/json`
- Body: OPT-format trace JSON

**Example:**
```bash
curl -X POST http://localhost:8080/trace \
     -H "Content-Type: text/plain" \
     -d 'int main() { int x = 5; return 0; }' | jq
```

### GET /health

Health check endpoint.

**Response:**
```json
{
  "status": "ok",
  "plugin": "/path/to/libInspectorPlugin.so",
  "runtime": "/path/to/libinspector_runtime.a",
  "plugin_exists": true,
  "runtime_exists": true
}
```

### GET /

Simple status page with usage information.

## Configuration

Environment variables:

| Variable           | Default                   | Description                    |
|--------------------|---------------------------|--------------------------------|
| `PORT`             | 8080                      | HTTP server port               |
| `HOST`             | 0.0.0.0                   | HTTP server host               |
| `TIMEOUT`          | 10                        | Execution timeout (seconds)    |
| `MAX_SOURCE_SIZE`  | 1048576                   | Max source size (bytes, 1MB)   |
| `INSPECTOR_PLUGIN` | cmake-build-debug/...     | Path to plugin .so             |
| `INSPECTOR_RUNTIME`| cmake-build-debug/...     | Path to runtime .a             |
| `INSPECTOR_INCLUDE`| runtime/                  | Path to include directory      |

## Error Responses

All errors return JSON with this structure:

```json
{
  "error": "error_type",
  "message": "Human-readable message",
  "details": "Additional details (optional)"
}
```

Error types:
- `empty_body` - No source code provided
- `body_too_large` - Source exceeds MAX_SOURCE_SIZE
- `instrumentation` - Clang plugin failed
- `compilation` - Compilation failed
- `execution` - Program execution failed
- `timeout` - Operation exceeded time limit
- `output` - Invalid trace output

## CORS

The server includes CORS headers for browser-based frontends:
- `Access-Control-Allow-Origin: *`
- `Access-Control-Allow-Methods: GET, POST, OPTIONS`

## Future Work

- WebSocket streaming for real-time trace updates
- Rate limiting
- Authentication
- Trace caching
- Batch processing
