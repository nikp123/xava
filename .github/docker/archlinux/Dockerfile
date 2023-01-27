FROM archlinux:latest

LABEL org.opencontainers.image.source https://github.com/nikp123/xava

WORKDIR /mnt

# Update Arch
RUN yes | pacman -Syyu --noconfirm

# Install XAVA dependencies
RUN yes | pacman -S --noconfirm make base-devel fftw alsa-lib pulseaudio \
    libx11 libxrender libxrandr sdl2 portaudio cmake git wayland-protocols \
    pipewire cairo wget sndio glew curl taglib imagemagick librsvg rust

# Download and install linuxdeploy
RUN wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
RUN chmod +x linuxdeploy-x86_64.AppImage
RUN sudo mv linuxdeploy-x86_64.AppImage /usr/local/bin/linuxdeploy

# Clean the package cache (without --noconfirm as it has N as default)
RUN yes | pacman -Scc

