project(xava C CXX)
cmake_minimum_required(VERSION 3.1)

option(SHMEM "SHMEM" OFF)


if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_STANDARD 11)
set(xava_VERSION_MAJOR 0)
set(xava_VERSION_MINOR 6)
set(xava_VERSION_PATCH 2)
set(xava_VERSION_TWEAK 3)

add_definitions(-DPACKAGE="xava" -DVERSION="${xava_VERSION_MAJOR}.${xava_VERSION_MINOR}.${xava_VERSION_PATCH}.${xava_VERSION_TWEAK}")
add_definitions(-DPREFIX="${CMAKE_INSTALL_PREFIX}")

find_package(PkgConfig REQUIRED)

# iniparser
find_library(INIPARSER iniparser)
if(NOT INIPARSER)
	message(STATUS "iniparser not found on system, building from source.")
	if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/.git)
		execute_process(COMMAND git submodule update --init)
		add_library(iniparser
				lib/iniparser/src/dictionary.c
				lib/iniparser/src/iniparser.c)
		add_definitions(-DINIPARSER)
	else()
		message(FATAL_ERROR "Cannot build iniparser from source since the project \
is not being built inside a git repository.\nIf you want iniparser \
to be built automatically, clone this repository instead of downloading it.")
	endif()
endif()

if(DEFINE_LEGACYINIPARSER AND INIPARSER)
	add_definitions(-DLEGACYINIPARSER)
endif()

# math lib
find_library(MATH_LIB m)
if(NOT MATH_LIB)
  message(FATAL_ERROR "math library not found")
endif()

# pthread
find_library(THREAD_LIB pthread)
if(NOT THREAD_LIB)
	message(FATAL_ERROR "pthread library not found")
endif()

# fftw3
pkg_check_modules(FFTW3 REQUIRED fftw3)
list(APPEND INCLUDE_DIRS "${FFTW3_INCLUDE_DIRS}")
list(APPEND LINK_DIRS "${FFTW3_LIBRARY_DIRS}")


# Optional features
set(ADDITIONAL_SOURCES "")
set(ADDITIONAL_LIBRARIES "")

# shmem
if(SHMEM)
	add_definitions(-DSHMEM)
	list(APPEND ADDITIONAL_LIBRARIES "-lrt")
	list(APPEND ADDITIONAL_SOURCES "src/input/shmem.c")
endif()

# OS-Specific inclusions
if(NOT (MSYS OR MINGW OR MSVC))
	message(STATUS "Not a Windows platform, can use POSIX now!")
	list(APPEND ADDITIONAL_SOURCES "src/input/fifo.c")
endif()

# SDL2
pkg_check_modules(SDL2 QUIET sdl2)
if(SDL2_FOUND)
	list(APPEND ADDITIONAL_SOURCES "src/output/graphical_sdl.c")
	list(APPEND INCLUDE_DIRS "${SDL2_INCLUDE_DIRS}")
	list(APPEND LINK_DIRS "${SDL2_LIBRARY_DIRS}")
	list(APPEND ADDITIONAL_LIBRARIES "${SDL2_LIBRARIES}")
	add_definitions(-DSDL)
else()
	message(STATUS "SDL2 library not found")
endif()

# Xorg
pkg_check_modules(X11 QUIET x11 xfixes)
if(X11_FOUND)
	add_definitions(-DXLIB)
	list(APPEND ADDITIONAL_SOURCES "src/output/graphical_x.c")
	list(APPEND INCLUDE_DIRS "${X11_INCLUDE_DIRS}")
	list(APPEND LINK_DIRS "${X11_LIBRARY_DIRS}")
	list(APPEND ADDITIONAL_LIBRARIES "${X11_LIBRARIES}")
	
	# GLX
	pkg_check_modules(GL QUIET gl xrender)
	if(GL_FOUND)
		add_definitions(-DGLX -DGL)
		list(APPEND INCLUDE_DIRS "${GL_INCLUDE_DIRS}")
		list(APPEND LINK_DIRS "${GL_LIBRARY_DIRS}")
		list(APPEND ADDITIONAL_LIBRARIES "${GL_LIBRARIES}")
	else()
		message(STATUS "GL and or Xrender library not found")
	endif()
else()
	message(STATUS "X11 and/or Xfixes library not found")
endif()

# winapi
if(MSYS OR MINGW OR MSVC)
	find_library(GDI_LIB gdi32)
	if(GDI_LIB)
		find_library(WGL_LIB opengl32)
		if(WGL_LIB)
			find_library(GLU_LIB glu32)
			if(GLU_LIB)
				find_library(DWM_LIB dwmapi)
				if(DWM_LIB)
					add_definitions(-DWIN -DGL)
					list(APPEND ADDITIONAL_SOURCES "src/output/graphical_win.c")
					list(APPEND ADDITIONAL_SOURCES "src/input/wasapi.cpp")
					list(APPEND ADDITIONAL_LIBRARIES "-lgdi32")
					list(APPEND ADDITIONAL_LIBRARIES "-lopengl32")
					list(APPEND ADDITIONAL_LIBRARIES "-lglu32")
					list(APPEND ADDITIONAL_LIBRARIES "-ldwmapi")
				else()
					message("DWMAPI library not found")
				endif()
			else()
				message("GLU library not found")
			endif()
		else()
			message("openGL library not found")
		endif()
	else()
		message(STATUS "GDI library not found")
	endif()
	
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		message(STATUS "Since release build, console is being disabled")
		SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS} -mwindows")
	endif()
endif()

# alsa
pkg_check_modules(ALSA QUIET alsa)
if(ALSA_FOUND)
	add_definitions(-DALSA)
	list(APPEND INCLUDE_DIRS "${ALSA_INCLUDE_DIRS}")
	list(APPEND LINK_DIRS "${ALSA_LIBRARY_DIRS}")
	list(APPEND ADDITIONAL_SOURCES "src/input/alsa.c")
	list(APPEND ADDITIONAL_LIBRARIES "${ALSA_LIBRARIES}")
else()
	message(STATUS "alsa library not found")
endif()

# pulseaudio
pkg_check_modules(PULSEAUDIO QUIET libpulse libpulse-simple)
if(PULSEAUDIO_FOUND)
	add_definitions(-DPULSE)
	list(APPEND INCLUDE_DIRS "${PULSEAUDIO_INCLUDE_DIRS}")
	list(APPEND LINK_DIRS "${PULSEAUDIO_LIBRARY_DIRS}")
	list(APPEND ADDITIONAL_SOURCES "src/input/pulse.c")
	list(APPEND ADDITIONAL_LIBRARIES "${PULSEAUDIO_LIBRARIES}")
else()
	message(STATUS "pulseaudio library not found")
endif()

# sndio
find_library(SNDIO_LIB sndio)
if(SNDIO_LIB)
	add_definitions(-DSNDIO)
	list(APPEND ADDITIONAL_SOURCES "src/input/sndio.c")
	list(APPEND ADDITIONAL_LIBRARIES "-lsndio")
else()
	message(STATUS "sndio library not found")
endif()

# portaudio
pkg_check_modules(PORTAUDIO QUIET portaudio-2.0)
if(PORTAUDIO_FOUND)
	add_definitions(-DPORTAUDIO)
	list(APPEND INCLUDE_DIRS "${PORTAUDIO_INCLUDE_DIRS}")
	list(APPEND LINK_DIRS "${PORTAUDIO_LIBRARY_DIRS}")
	list(APPEND ADDITIONAL_SOURCES "src/input/portaudio.c")
	list(APPEND ADDITIONAL_LIBRARIES "${PORTAUDIO_LIBRARIES}")
else()
	message(STATUS "portaudio library not found")
endif()

# Shitty workaround that doesnt work for BSD, Minix or anything with any amount of POSIX compatibility
# Blame: https://stackoverflow.com/questions/9160335/os-specific-instructions-in-cmake-how-to
# Also CMAKE_SYSTEM_NAME doesn't work, so I've tried my options. 
# Please fix this whoever has time and reads this.
if(UNIX AND NOT APPLE)
	list(APPEND ADDITIONAL_SOURCES "src/misc/inode_watcher.c")
endif()

# Build executable
add_executable(
	xava
	${ADDITIONAL_SOURCES}
	src/output/graphical.c
	src/output/raw.c
	src/config.c
	src/xava.c
)

include_directories(${INCLUDE_DIRS})
link_directories(${LINK_DIRS})

# Link executable
target_link_libraries(xava ${ADDITIONAL_LIBRARIES} m ${FFTW3_LIBRARIES} pthread iniparser)

# Install
install (TARGETS xava DESTINATION bin)
install (FILES LICENSE DESTINATION share/licenses/xava)
install (FILES assets/linux/xava.desktop DESTINATION share/applications)
