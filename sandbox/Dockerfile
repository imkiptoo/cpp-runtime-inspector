# C++ Runtime Inspector Sandbox Container
# Provides secure execution environment for tracing C++ programs.
#
# Build: docker build -t cpp-inspector-sandbox .
# Run: docker run --rm -i cpp-inspector-sandbox < input.cpp > trace.json

FROM ubuntu:24.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    clang-18 \
    llvm-18-dev \
    libclang-18-dev \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Set clang-18 as default
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100

# Copy source code
WORKDIR /build
COPY . .

# Build the project
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_COMPILER=clang \
    && cmake --build build

# Runtime image
FROM ubuntu:24.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y --no-install-recommends \
    clang-18 \
    libstdc++-13-dev \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100

# Copy built artifacts
COPY --from=builder /build/build/libInspectorPlugin.so /opt/inspector/lib/
COPY --from=builder /build/build/libinspector_runtime.a /opt/inspector/lib/
COPY --from=builder /build/runtime/inspector_runtime.h /opt/inspector/include/
COPY --from=builder /build/runtime/inspector/ /opt/inspector/include/inspector/
COPY --from=builder /build/sandbox/run-traced.sh /opt/inspector/bin/

# Create working directory
WORKDIR /workspace

# Set environment
ENV INSPECTOR_LIB=/opt/inspector/lib
ENV INSPECTOR_INCLUDE=/opt/inspector/include

# Entry point
ENTRYPOINT ["/opt/inspector/bin/run-traced.sh"]
