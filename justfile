# C++ Runtime Inspector — task runner
# Usage: just <verb> <noun>   e.g.  just run backend, just build release, just test web
# Run `just` to list recipes.

# --- configuration ---------------------------------------------------------

root          := justfile_directory()
web_dir       := root / "web"
inspector_inc := root / "core/runtime"

# Backend (services/api/server.py) settings — override via env.
backend_port := env_var_or_default("PORT", "8090")
backend_host := env_var_or_default("HOST", "0.0.0.0")

# --- meta ------------------------------------------------------------------

# List available recipes (default).
default:
    @just --list

alias help := default

# --- build <debug|release|web> --------------------------------------------

# Build the Clang plugin + runtime, or the web frontend. Default: debug.
build target="debug":
    #!/usr/bin/env bash
    set -euo pipefail
    case "{{target}}" in
      debug)
        cmake --preset default
        cmake --build --preset default ;;
      release)
        cmake --preset release
        cmake --build --preset release ;;
      web)
        cd {{web_dir}} && npm install && npm run build ;;
      *) echo "build: unknown target '{{target}}' (debug|release|web)" >&2; exit 2 ;;
    esac

# --- run <backend|web|example|docker> -------------------------------------

# Run a component. Default: backend.
run target="backend":
    #!/usr/bin/env bash
    set -euo pipefail
    case "{{target}}" in
      backend)
        # Build first; set INSPECTOR_ROOT/INSPECTOR_INCLUDE since server.py defaults resolve wrong.
        just build debug
        INSPECTOR_ROOT={{root}} INSPECTOR_INCLUDE={{inspector_inc}} \
          python3 {{root}}/services/api/server.py --host {{backend_host}} --port {{backend_port}} ;;
      web)
        # Pass the backend port so Vite's proxy targets the running backend.
        cd {{web_dir}} && npm install && BACKEND_PORT={{backend_port}} npm run dev ;;
      example)
        {{root}}/scripts/instrument-and-run.sh ;;
      docker)
        docker compose -f {{root}}/deploy/docker-compose.yml up --build ;;
      *) echo "run: unknown target '{{target}}' (backend|web|example|docker)" >&2; exit 2 ;;
    esac

# --- test <golden|web|update> ---------------------------------------------

# Run tests. Default: golden (end-to-end C++ traces).
test target="golden":
    #!/usr/bin/env bash
    set -euo pipefail
    case "{{target}}" in
      golden) cmake --build --preset default --target golden-tests ;;
      web)    cd {{web_dir}} && npm install && npm run test ;;
      update) {{root}}/scripts/update-expected.sh ;;
      *) echo "test: unknown target '{{target}}' (golden|web|update)" >&2; exit 2 ;;
    esac

# --- misc ------------------------------------------------------------------

# Smoke-test a running backend (/health + a /trace request). Override port with PORT=...
check:
    @curl -s http://localhost:{{backend_port}}/health
    @echo
    @curl -s -X POST http://localhost:{{backend_port}}/trace \
        -H "Content-Type: text/plain" \
        -d 'int main() { int x = 5; return 0; }'
    @echo

# Stop the docker compose stack.
stop:
    docker compose -f {{root}}/deploy/docker-compose.yml down

# Remove build dirs and generated trace artifacts.
clean:
    rm -rf {{root}}/cmake-build-debug {{root}}/cmake-build-release {{root}}/build
    rm -f {{root}}/trace.json {{root}}/tests/examples/*.instrumented.cpp
