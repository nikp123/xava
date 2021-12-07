project(xava C CXX)
cmake_minimum_required(VERSION 3.13)

# Include common funnctions
include(src/cmake/utilities.cmake)

# Fix/handle common build errors
include(src/cmake/build-fix.cmake)

# Debug is default
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

# Allow for custom link prefixes (in order to optimize CMake builds for packagers)
if(NOT XAVA_LINK_TO_INSTALL_DIR)
	set(XAVA_LINK_TO_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
endif()

# Define XAVA version
set(xava_VERSION_MAJOR 0)
set(xava_VERSION_MINOR 7)
set(xava_VERSION_PATCH 1)
set(xava_VERSION_TWEAK 1)
set(xava_VERSION "${xava_VERSION_MAJOR}.${xava_VERSION_MINOR}.${xava_VERSION_PATCH}.${xava_VERSION_TWEAK}")

# Definitions
add_definitions(-DPACKAGE="xava" -DVERSION="${xava_VERSION}")
add_definitions(-DXAVA_VERSION_MAJOR"=${xava_VERSION_MAJOR}")
add_definitions(-DXAVA_VERSION_MINOR"=${xava_VERSION_MINOR}")
add_definitions(-DXAVA_VERSION_PATCH"=${xava_VERSION_PATCH}")
add_definitions(-DXAVA_VERSION_TWEAK"=${xava_VERSION_TWEAK}")
add_definitions(-DPREFIX="${XAVA_LINK_TO_INSTALL_DIR}")

# Do OS-specific stuff
include(src/cmake/os-prerequisites.cmake)

# Set appropriate C flags
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-Wall ${BUILD_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG ${BUILD_DEBUG_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "-O3 -fvisibility='internal'")
set(CMAKE_CXX_FLAGS "-Wall ${BUILD_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG ${BUILD_DEBUG_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fvisibility='internal' ")

find_package(PkgConfig REQUIRED)

# math lib
find_library(MATH_LIB m HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
if(NOT MATH_LIB)
	message(FATAL_ERROR "math library not found")
endif()

# pthread
find_library(THREAD_LIB pthread HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
if(NOT THREAD_LIB)
	message(FATAL_ERROR "pthread library not found")
endif()

# Build shared library
include("src/shared/build.cmake")

# Build modules
include("src/cmake/modules.cmake")

# Build executable
add_executable(
	xava
	${ADDITIONAL_OBJS}
	src/config.c
	src/xava.c
)

# Link executable
target_link_libraries(xava xava-shared m pthread)

include(src/cmake/install_and_configure.cmake)

