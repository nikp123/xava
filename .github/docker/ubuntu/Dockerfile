# Ubuntu 22.04 LTS
#
# We build on an old, stable Ubuntu so the bundled libraries (and the final
# AppImage) only require an old glibc. Ubuntu 22.04 ships glibc 2.35, which
# becomes the AppImage's minimum glibc requirement. This keeps the artifact
# runnable on the widest range of distributions (unlike a rolling distro
# like Arch, where `pacman -Syyu` keeps dragging in brand-new glibc symbols).

FROM ubuntu:22.04

LABEL org.opencontainers.image.source=https://github.com/nikp123/xava

WORKDIR /mnt

# Install XAVA build dependencies. Loose list is fine: virtually all module
# dependencies are optional (probed with pkg_check_modules) — only fftw3f is
# hard-required.
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    wget \
    ca-certificates \
    sudo \
    file \
    # Basic
    libfftw3-dev \
    # Input
    libasound2-dev \
    libpulse-dev \
    libsndio-dev \
    portaudio19-dev \
    libpipewire-0.3-dev \
    # Output
    libsdl2-dev \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    libgles2-mesa-dev \
    libglew-dev \
    libx11-dev \
    libxrender-dev \
    libxrandr-dev \
    libxext-dev \
    libxfixes-dev \
    libxdmcp-dev \
    libwayland-dev \
    wayland-protocols \
    libcairo2-dev \
    # media_info module (cairo, taglib, zlib, curl, dbus)
    libtag1-dev \
    zlib1g-dev \
    libcurl4-openssl-dev \
    libdbus-1-dev \
    # Icons
    imagemagick \
    librsvg2-bin \
    # Rust bits
    rustc \
    cargo \
    && rm -rf /var/lib/apt/lists/*

# Download and install linuxdeploy. The distro has no real package, so fetch
# the upstream release; it bundles the `appimage` output plugin + runtime.
RUN wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
    && chmod +x linuxdeploy-x86_64.AppImage \
    && mv linuxdeploy-x86_64.AppImage /usr/local/bin/linuxdeploy
