cmake_minimum_required(VERSION 3.0)
project(cava)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD 11)
set(cava_VERSION_MAJOR 0)
set(cava_VERSION_MINOR 6)
set(cava_VERSION_PATCH 1)
set(cava_VERSION_TWEAK 0)

add_definitions(-DPACKAGE="CAVA" -DVERSION="$cava_VERSION_MAJOR.$cava_VERSION_MINOR.$cava_VERSION_PATH.$cava_VERSION_TWEAK")

# iniparser
find_library(INIPARSER iniparser)
if(NOT INIPARSER)
	message(STATUS "iniparser not found on system, building from source.")
	add_library(iniparser
			iniparser/src/dictionary.c
			iniparser/src/iniparser.c)
	add_definitions(-DINIPARSER)
endif()

# Math lib
find_library(MATH_LIB m)
if(NOT MATH_LIB)
  message(FATAL_ERROR "math library not found")
endif()

# fftw3
find_library(FFTW_LIB fftw3)
if(NOT FFTW_LIB)
	message(FATAL_ERROR "fftw library not found")
endif()

find_library(THREAD_LIB pthread)
if(NOT THREAD_LIB)
	message(FATAL_ERROR "pthread library not found")
endif()

# Build executable
add_executable(
	cava
	output/graphical.c
	output/raw.c
	cava.c
)

# Link executable
target_link_libraries(cava m fftw3 pthread iniparser)
