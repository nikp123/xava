cmake_minimum_required(VERSION 3.0)
project(cava)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD 11)
set(cava_VERSION_MAJOR 0)
set(cava_VERSION_MINOR 6)
set(cava_VERSION_PATCH 1)
set(cava_VERSION_TWEAK 0)

add_definitions(-DPACKAGE="cava" -DVERSION="${cava_VERSION_MAJOR}.${cava_VERSION_MINOR}.${cava_VERSION_PATCH}.${cava_VERSION_TWEAK}")

# iniparser
find_library(INIPARSER iniparser)
if(NOT INIPARSER)
	message(STATUS "iniparser not found on system, building from source.")
	add_library(iniparser
			iniparser/src/dictionary.c
			iniparser/src/iniparser.c)
	add_definitions(-DINIPARSER)
endif()

# math lib
find_library(MATH_LIB m)
if(NOT MATH_LIB)
  message(FATAL_ERROR "math library not found")
endif()

# fftw3
find_library(FFTW_LIB fftw3)
if(NOT FFTW_LIB)
	message(FATAL_ERROR "fftw library not found")
endif()

# pthread
find_library(THREAD_LIB pthread)
if(NOT THREAD_LIB)
	message(FATAL_ERROR "pthread library not found")
endif()


# Optional features

# ncursesw
find_library(CURSES_LIB ncursesw)
if(CURSES_LIB)
	add_definitions(-DNCURSES)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lncursesw")
else()
	message(STATUS "ncursesw library not found")
endif()

# SDL2
find_library(SDL_LIB SDL2)
if(SDL_LIB)
	add_definitions(-DSDL)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lSDL2")
else()
	message(STATUS "SDL2 library not found")
endif()

# Xorg
find_library(XORG_LIB X11)
if(XORG_LIB)
	find_library(XFIXES_LIB Xfixes)
	if(XFIXES_LIB)
		add_definitions(-DXLIB)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lX11 -lXfixes")
		
		# GLX
		find_library(GL_LIB GL)
		if(GL_LIB)
			add_definitions(-DGLX -DGL)
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lGL")
		else()
			message(STATUS "GL library not found")
		endif()
	else()
		message(STATUS "Xfixes library not found")
	endif()
else()
	message(STATUS "X11 library not found")
endif()

# alsa
find_library(ALSA_LIB asound)
if(ALSA_LIB)
	add_definitions(-DALSA)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lasound")
else()
	message(STATUS "alsa library not found")
endif()

# pulseaudio
find_library(PULSE_LIB pulse-simple)
if(PULSE_LIB)
	add_definitions(-DPULSE)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpulse-simple -lpulse")
else()
	message(STATUS "pulseaudio library not found")
endif()

# sndio
find_library(SNDIO_LIB sndio)
if(SNDIO_LIB)
	add_definitions(-DSNDIO)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lsndio")
else()
	message(STATUS "sndio library not found")
endif()

# portaudio
find_library(PORTAUDIO_LIB portaudio)
if(PORTAUDIO_LIB)
	add_definitions(-DPORTAUDIO)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lportaudio")
else()
	message(STATUS "portaudio library not found")
endif()

# Build executable
add_executable(
	cava
	output/graphical.c
	output/raw.c
	cava.c
)

if(CURSES_LIB)
endif()

# Link executable
target_link_libraries(cava m fftw3 pthread iniparser)
