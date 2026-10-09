# syntax=docker/dockerfile:1
#
# Stage "build"      : compiles the game for Linux AND cross-compiles it for Windows.
# Stage "artifacts"  : contains only the finished binaries (used with --output).
# Stage "run-linux"  : small image that can run the game on a Linux desktop (X11).

FROM ubuntu:24.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake git ca-certificates \
        libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev \
        mingw-w64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt trex.cpp ./
COPY cmake ./cmake

# Linux binary
RUN cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build-linux -j"$(nproc)"

# Windows .exe (cross-compiled, fully static)
RUN cmake -S . -B build-win -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake \
 && cmake --build build-win -j"$(nproc)"

RUN mkdir -p /out/linux /out/windows \
 && cp build-linux/bin/dino_runner     /out/linux/ \
 && cp build-win/bin/dino_runner.exe   /out/windows/

# ---- just the binaries -------------------------------------------------------
FROM scratch AS artifacts
COPY --from=build /out /

# ---- optional: run the Linux build inside a container ------------------------
FROM ubuntu:24.04 AS run-linux
RUN apt-get update && apt-get install -y --no-install-recommends \
        libx11-6 libgl1 libgl1-mesa-dri libxrandr2 libxcursor1 libxi6 libxinerama1 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=build /out/linux/dino_runner /usr/local/bin/dino_runner
ENV LIBGL_ALWAYS_SOFTWARE=1
CMD ["dino_runner"]
