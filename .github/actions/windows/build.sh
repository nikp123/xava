#!/bin/sh

# CPU architecture
arch=$1

rm -rf release
rm -rf debug

# Bail on error
set -e

build_args="-j$(nproc)"
cmake_args='-DCMAKE_INSTALL_PREFIX=/usr'

# toolchain prefixes because C
tool_prefix="$arch-w64-mingw32"

echo "Testing CMake on $arch"
$tool_prefix-cmake --version

# Try debug build
mkdir debug
cd debug
    echo "Trying debug for $arch"
    $tool_prefix-cmake "$cmake_args" -DCMAKE_BUILD_TYPE=Debug -Werror=dev ..
    make $build_args
cd ..
rm -rf debug

# Release build
mkdir release
cd release
    echo "Trying release for $arch"
    $tool_prefix-cmake "$cmake_args" -DCMAKE_BUILD_TYPE=Release -Werror=dev ..
    make $build_args

    # Make installer and move it to the last directory
    echo "Buidling installer for $arch"
    makensis xava.nsi
    mv xava-win-installer.exe ../xava-installer-$arch.exe
cd ..
rm -rf release

