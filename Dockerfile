# C++ Runtime Inspector - Docker Build
# Multi-stage build for minimal final image

# ============================================================================
# Stage 1: Build the plugin and runtime
# ============================================================================
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install prerequisites and add LLVM repository
RUN apt-get update && apt-get install -y \
    wget \
    gnupg \
    software-properties-common \
    && wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc \
    && echo "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-17 main" >> /etc/apt/sources.list.d/llvm.list \
    && apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    llvm-17-dev \
    clang-17 \
    libclang-17-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set up LLVM paths
ENV LLVM_DIR=/usr/lib/llvm-17
ENV PATH="${LLVM_DIR}/bin:${PATH}"

# Copy source code
WORKDIR /build
COPY CMakeLists.txt ./
COPY plugin/ ./plugin/
COPY runtime/ ./runtime/
COPY shim/ ./shim/

# Build the project
RUN mkdir -p cmake-build && cd cmake-build && \
    cmake .. \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DLLVM_DIR=${LLVM_DIR}/lib/cmake/llvm \
        -DClang_DIR=${LLVM_DIR}/lib/cmake/clang && \
    ninja

# ============================================================================
# Stage 2: Runtime image with server
# ============================================================================
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install prerequisites and add LLVM repository
RUN apt-get update && apt-get install -y \
    wget \
    gnupg \
    software-properties-common \
    curl \
    && wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc \
    && echo "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-17 main" >> /etc/apt/sources.list.d/llvm.list \
    && apt-get update && apt-get install -y \
    clang-17 \
    llvm-17 \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Set up LLVM paths
ENV LLVM_DIR=/usr/lib/llvm-17
ENV PATH="${LLVM_DIR}/bin:${PATH}"

# Create app directory
WORKDIR /app

# Copy built artifacts from builder
COPY --from=builder /build/cmake-build/libInspectorPlugin.so ./lib/
COPY --from=builder /build/cmake-build/libinspector_runtime.a ./lib/

# Copy runtime headers and server
COPY runtime/ ./runtime/
COPY frontend-adapter/ ./frontend-adapter/

# Set environment variables for the server
ENV INSPECTOR_ROOT=/app
ENV INSPECTOR_PLUGIN=/app/lib/libInspectorPlugin.so
ENV INSPECTOR_RUNTIME=/app/lib/libinspector_runtime.a
ENV INSPECTOR_INCLUDE=/app/runtime
ENV CLANGXX=clang++-17

# Expose the server port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Run the server
CMD ["python3", "frontend-adapter/server.py", "--host", "0.0.0.0", "--port", "8080"]
