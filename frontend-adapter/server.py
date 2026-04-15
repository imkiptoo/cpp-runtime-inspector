#!/usr/bin/env python3
"""
C++ Runtime Inspector - Frontend Adapter HTTP Server

Accepts C++ source code via HTTP POST and returns OPT-format trace JSON.
This is the v1 HTTP-only implementation; WebSocket streaming is future work.

Usage:
    python server.py [--port 8080] [--host 0.0.0.0]

Endpoints:
    POST /trace   - Instrument and trace C++ source code
    GET /health   - Health check endpoint
    GET /         - Simple status page

Example:
    curl -X POST http://localhost:8080/trace \
         -H "Content-Type: text/plain" \
         -d 'int main() { int x = 5; return 0; }' | jq
"""

import argparse
import json
import logging
import os
import platform
import subprocess
import tempfile
import shutil
from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path
from typing import Optional

# Configuration
DEFAULT_PORT = 8080
DEFAULT_HOST = "0.0.0.0"
TIMEOUT = int(os.environ.get("TIMEOUT", 10))
MAX_SOURCE_SIZE = int(os.environ.get("MAX_SOURCE_SIZE", 1024 * 1024))  # 1MB

# Platform-specific plugin extension
PLUGIN_EXT = ".dylib" if platform.system() == "Darwin" else ".so"

# Paths - can be overridden via environment
INSPECTOR_ROOT = Path(os.environ.get(
    "INSPECTOR_ROOT",
    Path(__file__).parent.parent
))
PLUGIN_PATH = Path(os.environ.get(
    "INSPECTOR_PLUGIN",
    INSPECTOR_ROOT / "cmake-build-debug" / f"libInspectorPlugin{PLUGIN_EXT}"
))
RUNTIME_PATH = Path(os.environ.get(
    "INSPECTOR_RUNTIME",
    INSPECTOR_ROOT / "cmake-build-debug" / "libinspector_runtime.a"
))
INCLUDE_PATH = Path(os.environ.get(
    "INSPECTOR_INCLUDE",
    INSPECTOR_ROOT / "runtime"
))

# Use Homebrew LLVM clang++ on macOS (plugin requires matching LLVM version)
if platform.system() == "Darwin":
    CLANGXX = os.environ.get("CLANGXX", "/opt/homebrew/opt/llvm/bin/clang++")
else:
    CLANGXX = os.environ.get("CLANGXX", "clang++")

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s"
)
logger = logging.getLogger(__name__)


class TraceError(Exception):
    """Exception for trace generation errors."""
    def __init__(self, phase: str, message: str, details: Optional[str] = None):
        self.phase = phase
        self.message = message
        self.details = details
        super().__init__(f"{phase}: {message}")

    def to_json(self) -> dict:
        result = {
            "error": self.phase,
            "message": self.message
        }
        if self.details:
            result["details"] = self.details
        return result


def instrument_and_run(source_code: str) -> str:
    """
    Instrument, compile, and run C++ source code.

    Args:
        source_code: C++ source code to trace

    Returns:
        JSON trace string

    Raises:
        TraceError: If any step fails
    """
    workdir = tempfile.mkdtemp(prefix="inspector_")
    try:
        input_file = Path(workdir) / "input.cpp"
        instrumented_file = Path(workdir) / "input.cpp.instrumented.cpp"
        output_binary = Path(workdir) / "program"

        # Write source
        input_file.write_text(source_code)

        # Step 1: Instrument
        logger.info("Instrumenting source...")
        result = subprocess.run(
            [
                CLANGXX, "-std=c++17", "-fsyntax-only",
                f"-fplugin={PLUGIN_PATH}",
                f"-I{INCLUDE_PATH}",
                str(input_file)
            ],
            capture_output=True,
            text=True,
            timeout=TIMEOUT
        )
        if result.returncode != 0:
            raise TraceError(
                "instrumentation",
                "Failed to instrument source",
                result.stderr
            )

        if not instrumented_file.exists():
            raise TraceError(
                "instrumentation",
                "Plugin did not produce instrumented file"
            )

        # Step 2: Compile
        logger.info("Compiling instrumented code...")
        result = subprocess.run(
            [
                CLANGXX, "-std=c++17", "-O0", "-g",
                f"-I{INCLUDE_PATH}",
                str(instrumented_file),
                str(RUNTIME_PATH),
                "-o", str(output_binary)
            ],
            capture_output=True,
            text=True,
            timeout=TIMEOUT
        )
        if result.returncode != 0:
            raise TraceError(
                "compilation",
                "Failed to compile instrumented source",
                result.stderr
            )

        # Step 3: Run
        logger.info("Running program...")
        result = subprocess.run(
            [str(output_binary)],
            capture_output=True,
            text=True,
            timeout=TIMEOUT
        )

        # Trace is on stderr
        trace = result.stderr

        if not trace:
            raise TraceError(
                "execution",
                "Program did not produce trace output"
            )

        # Validate JSON
        try:
            json.loads(trace)
        except json.JSONDecodeError as e:
            raise TraceError(
                "output",
                "Invalid trace JSON",
                str(e)
            )

        return trace

    except subprocess.TimeoutExpired:
        raise TraceError(
            "timeout",
            f"Operation exceeded {TIMEOUT}s time limit"
        )
    finally:
        shutil.rmtree(workdir, ignore_errors=True)


class InspectorHandler(BaseHTTPRequestHandler):
    """HTTP request handler for the inspector API."""

    def _send_json(self, status: int, data: dict):
        """Send a JSON response."""
        body = json.dumps(data, indent=2).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", len(body))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def _send_text(self, status: int, text: str, content_type: str = "text/plain"):
        """Send a text response."""
        body = text.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", len(body))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def do_OPTIONS(self):
        """Handle CORS preflight requests."""
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Access-Control-Max-Age", "86400")
        self.end_headers()

    def do_GET(self):
        """Handle GET requests."""
        if self.path == "/health":
            self._send_json(200, {
                "status": "ok",
                "plugin": str(PLUGIN_PATH),
                "runtime": str(RUNTIME_PATH),
                "plugin_exists": PLUGIN_PATH.exists(),
                "runtime_exists": RUNTIME_PATH.exists()
            })
        elif self.path == "/":
            self._send_text(200, """
C++ Runtime Inspector - Frontend Adapter

Endpoints:
  POST /trace   - Submit C++ source, receive trace JSON
  GET /health   - Service health check

Example:
  curl -X POST http://localhost:8080/trace \\
       -H "Content-Type: text/plain" \\
       -d 'int main() { int x = 5; return 0; }'
""", "text/plain")
        else:
            self._send_json(404, {"error": "not_found"})

    def do_POST(self):
        """Handle POST requests."""
        if self.path != "/trace":
            self._send_json(404, {"error": "not_found"})
            return

        # Read body
        content_length = int(self.headers.get("Content-Length", 0))
        if content_length == 0:
            self._send_json(400, {"error": "empty_body"})
            return

        if content_length > MAX_SOURCE_SIZE:
            self._send_json(413, {
                "error": "body_too_large",
                "max_size": MAX_SOURCE_SIZE
            })
            return

        source_code = self.rfile.read(content_length).decode("utf-8")

        try:
            logger.info(f"Received trace request ({len(source_code)} bytes)")
            trace_json = instrument_and_run(source_code)

            # Parse and re-serialize to ensure consistent formatting
            trace_data = json.loads(trace_json)
            self._send_json(200, trace_data)

        except TraceError as e:
            logger.warning(f"Trace error: {e}")
            self._send_json(400, e.to_json())

        except Exception as e:
            logger.exception("Unexpected error")
            self._send_json(500, {
                "error": "internal_error",
                "message": str(e)
            })

    def log_message(self, format, *args):
        """Override to use our logger."""
        logger.info("%s - %s", self.address_string(), format % args)


def main():
    parser = argparse.ArgumentParser(
        description="C++ Runtime Inspector Frontend Adapter"
    )
    parser.add_argument(
        "--port", "-p",
        type=int,
        default=int(os.environ.get("PORT", DEFAULT_PORT)),
        help=f"Port to listen on (default: {DEFAULT_PORT})"
    )
    parser.add_argument(
        "--host",
        default=os.environ.get("HOST", DEFAULT_HOST),
        help=f"Host to bind to (default: {DEFAULT_HOST})"
    )
    args = parser.parse_args()

    # Validate paths
    if not PLUGIN_PATH.exists():
        logger.warning(f"Plugin not found at {PLUGIN_PATH}")
    if not RUNTIME_PATH.exists():
        logger.warning(f"Runtime not found at {RUNTIME_PATH}")

    server = HTTPServer((args.host, args.port), InspectorHandler)
    logger.info(f"Starting server on {args.host}:{args.port}")
    logger.info(f"Plugin: {PLUGIN_PATH}")
    logger.info(f"Runtime: {RUNTIME_PATH}")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        logger.info("Shutting down")
        server.shutdown()


if __name__ == "__main__":
    main()
