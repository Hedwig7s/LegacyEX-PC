FROM ubuntu:26.10

ENV DEBIAN_FRONTEND=noninteractive
ENV WORKSPACE=/workspace
WORKDIR ${WORKSPACE}

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    clang \
    clangd \
    cmake \
    lld \
    llvm \
    llvm-dev \
    llvm-runtime \
    mingw-w64 \
    ninja-build \
    python3 \
    python3-minimal \
    build-essential \
    git \
    make \
    pkg-config \
    gnome-desktop-testing \
    libasound2-dev \
    libpulse-dev \
    libaudio-dev \
    libfribidi-dev \
    libjack-dev \
    libsndio-dev \
    libx11-dev \
    libxext-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxfixes-dev \
    libxi-dev \
    libxss-dev \
    libxtst-dev \
    libxkbcommon-dev \
    libdrm-dev \
    libgbm-dev \
    libgl1-mesa-dev \
    libgles2-mesa-dev \
    libegl1-mesa-dev \
    libdbus-1-dev \
    libibus-1.0-dev \
    libudev-dev \
    libthai-dev \
    libusb-1.0-0-dev \
    libpipewire-0.3-dev \
    libwayland-dev \
    libdecor-0-dev \
    liburing-dev \
    ccache \
    && rm -rf /var/lib/apt/lists/*

COPY build.py /usr/local/bin/build
RUN chmod +x /usr/local/bin/build

RUN ln -s /usr/bin/llvm-windres-18 /usr/local/bin/llvm-windres

ENTRYPOINT ["/usr/local/bin/build"]
