# C++ Runtime Inspector - Frontend Adapter
#
# This container runs the HTTP adapter service that accepts C++ source
# and returns trace JSON.
#
# Build: docker build -t cpp-inspector-adapter .
# Run: docker run -p 8080:8080 cpp-inspector-adapter

FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 \
    python3-pip \
    clang-18 \
    libstdc++-13-dev \
    && rm -rf /var/lib/apt/lists/* \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100 \
    && update-alternatives --install /usr/bin/python python /usr/bin/python3 100

# Copy inspector artifacts
COPY cmake-build-release/libInspectorPlugin.so /opt/inspector/lib/
COPY cmake-build-release/libinspector_runtime.a /opt/inspector/lib/
COPY runtime/inspector_runtime.h /opt/inspector/include/
COPY runtime/inspector/ /opt/inspector/include/inspector/

# Copy adapter
COPY frontend-adapter/server.py /opt/inspector/adapter/
COPY frontend-adapter/requirements.txt /opt/inspector/adapter/

# Set environment
ENV INSPECTOR_PLUGIN=/opt/inspector/lib/libInspectorPlugin.so
ENV INSPECTOR_RUNTIME=/opt/inspector/lib/libinspector_runtime.a
ENV INSPECTOR_INCLUDE=/opt/inspector/include
ENV PORT=8080

WORKDIR /opt/inspector/adapter

EXPOSE 8080

CMD ["python", "server.py"]
