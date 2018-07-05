cmake_minimum_required(VERSION 3.1)
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
	execute_process(COMMAND git submodule update --init)
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
set(ADDITIONAL_SOURCES "")
set(ADDITIONAL_LIBRARIES "")

# OS-Specific inclusions
if(NOT (MSYS OR MINGW OR MSVC))
	messaage(STATUS "Not a Windows platform, can use POSIX now!")
	list(APPEND ADDITIONAL_SOURCES "output/terminal_noncurses.c")
	list(APPEND ADDITIONAL_SOURCES "input/fifo.c")
endif()

# ncursesw
find_library(CURSES_LIB ncursesw)
if(CURSES_LIB)
	add_definitions(-DNCURSES)
	list(APPEND ADDITIONAL_SOURCES "output/terminal_ncurses.c")
	list(APPEND ADDITIONAL_SOURCES "output/terminal_bcircle.c")
	list(APPEND ADDITIONAL_LIBRARIES "-lncursesw")
else()
	message(STATUS "ncursesw library not found")
endif()

# SDL2
find_library(SDL_LIB SDL2)
if(SDL_LIB)
	add_definitions(-DSDL)
	list(APPEND ADDITIONAL_SOURCES "output/graphical_sdl.c")
	list(APPEND ADDITIONAL_LIBRARIES "-lSDL2")
else()
	message(STATUS "SDL2 library not found")
endif()

# Xorg
find_library(XORG_LIB X11)
if(XORG_LIB)
	find_library(XFIXES_LIB Xfixes)
	if(XFIXES_LIB)
		add_definitions(-DXLIB)
		list(APPEND ADDITIONAL_SOURCES "output/graphical_x.c")
		list(APPEND ADDITIONAL_LIBRARIES "-lX11")
		list(APPEND ADDITIONAL_LIBRARIES "-lXfixes")
		
		# GLX
		find_library(GL_LIB GL)
		if(GL_LIB)
			add_definitions(-DGLX -DGL)
			list(APPEND ADDITIONAL_LIBRARIES "-lGL")
		else()
			message(STATUS "GL library not found")
		endif()
	else()
		message(STATUS "Xfixes library not found")
	endif()
else()
	message(STATUS "X11 library not found")
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
					list(APPEND ADDITIONAL_SOURCES "output/graphical_win.c")
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
endif()

# alsa
find_library(ALSA_LIB asound)
if(ALSA_LIB)
	add_definitions(-DALSA)
	list(APPEND ADDITIONAL_SOURCES "input/alsa.c")
	list(APPEND ADDITIONAL_LIBRARIES "-lasound")
else()
	message(STATUS "alsa library not found")
endif()

# pulseaudio
find_library(PULSE_LIB pulse-simple)
if(PULSE_LIB)
	add_definitions(-DPULSE)
	list(APPEND ADDITIONAL_SOURCES "input/pulse.c")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpulse-simple -lpulse")
	list(APPEND ADDITIONAL_LIBRARIES "-lpulse")
	list(APPEND ADDITIONAL_LIBRARIES "-lpulse-simple")
else()
	message(STATUS "pulseaudio library not found")
endif()

# sndio
find_library(SNDIO_LIB sndio)
if(SNDIO_LIB)
	add_definitions(-DSNDIO)
	list(APPEND ADDITIONAL_SOURCES "input/sndio.c")
	list(APPEND ADDITIONAL_LIBRARIES "-lsndio")
else()
	message(STATUS "sndio library not found")
endif()

# portaudio
find_library(PORTAUDIO_LIB portaudio)
if(PORTAUDIO_LIB)
	add_definitions(-DPORTAUDIO)
	list(APPEND ADDITIONAL_SOURCES "input/portaudio.c")
	list(APPEND ADDITIONAL_LIBRARIES "-lportaudio")
else()
	message(STATUS "portaudio library not found")
endif()

# Build executable
add_executable(
	cava
	${ADDITIONAL_SOURCES}
	output/graphical.c
	output/raw.c
	config.c
	cava.c
)

# Link executable
target_link_libraries(cava ${ADDITIONAL_LIBRARIES} m fftw3 pthread iniparser)
