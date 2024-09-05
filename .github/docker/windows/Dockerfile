FROM archlinux:latest

LABEL org.opencontainers.image.source=https://github.com/nikp123/xava

WORKDIR /mnt

# Append ownstuff's repo to the arch install
RUN echo $'\n\
[ownstuff]\n\
Server = https://martchus.no-ip.biz/repo/arch/$repo/os/$arch\n\
Server = https://ftp.f3l.de/~martchus/$repo/os/$arch\n\
SigLevel = Optional TrustAll' >> /etc/pacman.conf

# Update Arch
RUN yes | pacman -Syyu --noconfirm; yes | pacman -Scc

# Install basic C/C++ development tools
RUN yes | pacman --noconfirm -S binutils mingw-w64-binutils mingw-w64-crt \
    mingw-w64-headers mingw-w64-winpthreads mingw-w64-cmake mingw-w64-gcc \
    mingw-w64-extra-cmake-modules mingw-w64-make mingw-w64-pkg-config make \
    ; yes | pacman -Scc

# Install cross-compiler XAVA dependencies
RUN yes | pacman -S --noconfirm git mingw-w64-fftw nsis mingw-w64-sdl2 \
    mingw-w64-cairo mingw-w64-portaudio mingw-w64-winpthreads python \
    mingw-w64-glew rust mingw-w64-curl \
    ; yes | pacman -Scc

