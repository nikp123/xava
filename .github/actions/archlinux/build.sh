#!/bin/sh

rm -rf build

set -e
mkdir build

cd build

# Build AppImage version of XAVA
cmake .. -DUNIX_INDEPENDENT_PATHS=ON -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make install DESTDIR=AppDir

# Fix AppImage not running due to the lack of FUSE
export APPIMAGE_EXTRACT_AND_RUN=1

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

mv XAVA-$commit_sha-x86_64.AppImage xava-$commit_branch-x86_64.AppImage
chmod +x xava-$commit_branch-x86_64.AppImage

# Extract version info
cat CMakeCache.txt | grep xava_VERSION | cut -d'=' -f2 > version.txt
