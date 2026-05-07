# C++ Runtime Inspector Sandbox

Secure execution environment for tracing C++ programs.

## Overview

The sandbox provides isolated execution of untrusted C++ code with:

- **Resource limits**: CPU time, memory, process count
- **Syscall filtering**: seccomp profile restricts available system calls
- **Network isolation**: No network access
- **Filesystem isolation**: Read-only root, writable tmpfs for workspace
- **Privilege dropping**: All capabilities dropped, no privilege escalation

## Quick Start

### Build the container

```bash
docker build -t cpp-inspector-sandbox -f services/sandbox/Dockerfile .
```

### Run a trace

```bash
echo 'int main() { int x = 5; return 0; }' | docker run --rm -i cpp-inspector-sandbox > trace.json
```

### Using docker-compose

```bash
cd services/sandbox
docker-compose build
echo 'int main() { int x = 5; return 0; }' | docker-compose run --rm inspector > trace.json
```

## Configuration

Environment variables:

| Variable     | Default | Description              |
|--------------|---------|--------------------------|
| `TIMEOUT`    | 10      | Execution timeout (sec)  |
| `MAX_MEMORY` | 262144  | Memory limit (KB = 256MB)|

## Security Features

### Seccomp Profile

The `seccomp-profile.json` restricts syscalls to:

- **Allowed**: read, write (stdout/stderr only), mmap, brk, exit, signal handling
- **Blocked**: fork, exec, network, filesystem writes, ptrace

### Docker Security Options

```yaml
security_opt:
  - no-new-privileges:true
  - seccomp:seccomp-profile.json
cap_drop:
  - ALL
network_mode: none
read_only: true
```

## Exit Codes

| Code | Meaning                    |
|------|----------------------------|
| 0    | Success                    |
| 1    | Empty input                |
| 2    | Instrumentation failed     |
| 3    | No instrumented file       |
| 4    | Compilation failed         |
| 5    | Timeout                    |
| 6+   | Program exit code          |

## Files

- `Dockerfile` - Multi-stage build for minimal runtime image
- `run-traced.sh` - Entry point script for instrument/compile/run
- `seccomp-profile.json` - Syscall whitelist for seccomp
- `docker-compose.yml` - Compose configuration with security settings
