#!/bin/sh

rm -rf build

set -e

# Fix build issue from Actions
git config --global --add safe.directory /github/workspace

# Build dir
mkdir build
cd build

# Build AppImage version of XAVA
cmake .. -DUNIX_INDEPENDENT_PATHS=ON -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make install DESTDIR=AppDir

# Fix AppImage not running due to the lack of FUSE
export APPIMAGE_EXTRACT_AND_RUN=1

# Fix AppImage not building due to stripping incompatibilities with ArchLinux
# https://github.com/linuxdeploy/linuxdeploy/issues/272
export NO_STRIP=true

# Create AppImage
env LD_LIBRARY_PATH=. linuxdeploy \
	--appdir AppDir \
	--output appimage \
	--icon-filename AppDir/usr/share/icons/hicolor/scalable/apps/xava.svg \
	--desktop-file AppDir/usr/share/applications/xava.desktop \
	-llibxava-shared.so

# Fix the filename
commit_branch="$(echo ${GITHUB_REF#refs/heads/})"
commit_sha="$(git rev-parse --short HEAD)"

mv XAVA-x86_64.AppImage xava-x86_64.AppImage
chmod +x xava-x86_64.AppImage

# Extract version info
cat CMakeCache.txt | grep xava_VERSION | cut -d'=' -f2 > version.txt

# Leave directory
cd ..

