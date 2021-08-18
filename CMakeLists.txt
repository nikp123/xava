project(xava C CXX)
cmake_minimum_required(VERSION 3.13)

# Concatenate files in CMake
function(cat IN_FILE OUT_FILE)
	file(READ ${IN_FILE} CONTENTS)
	file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

# REQUIRE GIT to be present
if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/.git)
	message(FATAL_ERROR "XAVA from 0.7.0 onwards requires to be built in a .git directory")
endif()

# Fix pkg-config for cross-builds (such as MinGW on ArchLinux)
if(CMAKE_FIND_ROOT_PATH)
	set(CMAKE_SYSROOT "${CMAKE_FIND_ROOT_PATH}")

	set(ENV{PKG_CONFIG_DIR} "")
	set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/lib/pkgconfig:${CMAKE_SYSROOT}/share/pkgconfig")
	set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
endif()

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
set(xava_VERSION_PATCH 0)
set(xava_VERSION_TWEAK 0)
set(xava_VERSION "${xava_VERSION_MAJOR}.${xava_VERSION_MINOR}.${xava_VERSION_PATCH}.${xava_VERSION_TWEAK}")

# Definitions
add_definitions(-DPACKAGE="xava" -DVERSION="${xava_VERSION}")
add_definitions(-DPREFIX="${XAVA_LINK_TO_INSTALL_DIR}")

# Windows-y things
if(MINGW)
	set(CMAKE_FIND_LIBRARY_SUFFIXES .dll ${CMAKE_FIND_LIBRARY_SUFFIXES})
	add_definitions(-DXAVA_DEFAULT_INPUT="wasapi" -DXAVA_DEFAULT_OUTPUT="win")

	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		message(STATUS "Since release build, console is being disabled")
		SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS} -mwindows")

		# Prepare NSI file for compilation
		#file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/example_files" xava_EXAMPLE_FILES)
		string(REPLACE "/" "\\" xava_EXAMPLE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/example_files")
		string(REPLACE "/" "\\" xava_SOURCE_DIR    "${CMAKE_CURRENT_SOURCE_DIR}")
		configure_file("assets/windows/xava.nsi.template" "xava.nsi" NEWLINE_STYLE CRLF)
	else()
		set(CMAKE_SUPPORT_WINDOWS_EXPORT_ALL_SYMBOLS 1)
	endif()

	set(xava_EXECUTABLE_WINDOWS_ICO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/windows/xava.ico")
	configure_file("assets/windows/xava.rc.template" "xava.rc" NEWLINE_STYLE CRLF)

	set(ADDITIONAL_OBJS ${CMAKE_CURRENT_BINARY_DIR}/xava.rc)

	# This is to prevent DLL hell, also helps a fuckton with packaging
	set(CMAKE_SHARED_LINKER_FLAGS "-static")
else()
	# This hack is a of courtesy of: https://stackoverflow.com/a/40947954/3832385
	string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
	add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")
	set(BUILD_DEBUG_FLAGS "-rdynamic")
	set(CMAKE_SHARED_LINKER_FLAGS "-rdynamic")
endif()


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

# Shitty workaround that doesnt work for BSD, Minix or anything with any amount of POSIX compatibility
# Blame: https://stackoverflow.com/questions/9160335/os-specific-instructions-in-cmake-how-to
# Also CMAKE_SYSTEM_NAME doesn't work, so I've tried my options.
# Please fix this whoever has time and reads this.
if(UNIX AND NOT APPLE)
	# Add proper version info to the desktop shortcut file
	configure_file("assets/linux/xava.desktop.template" "xava.desktop" NEWLINE_STYLE UNIX)
endif()

# Build filter modules
file(GLOB filters "src/filters/*/build.cmake" )
foreach(dir ${filters})
	get_filename_component(XAVA_MODULE_DIR ${dir} DIRECTORY)
	include(${dir})
endforeach()

# Build output modules
file(GLOB outputs "src/output/*/build.cmake" )
foreach(dir ${outputs})
	get_filename_component(XAVA_MODULE_DIR ${dir} DIRECTORY)
	include(${dir})
endforeach()

# Build unsupported output modules
if(BUILD_LEGACY_OUTPUTS)
	file(GLOB legacy_outputs "src/output/unsupported/*/build.cmake" )
	foreach(dir ${legacy_outputs})
		get_filename_component(XAVA_MODULE_DIR ${dir} DIRECTORY)
		include(${dir})
	endforeach()
endif()

# Build input modules
file(GLOB inputs "src/input/*/build.cmake" )
foreach(dir ${inputs})
	get_filename_component(XAVA_MODULE_DIR ${dir} DIRECTORY)
	include(${dir})
endforeach()

include("src/shared/build.cmake")

# Build executable
add_executable(
	xava
	${ADDITIONAL_OBJS}
	src/config.c
	src/xava.c
)

# Link executable
target_link_libraries(xava xava-shared m pthread)

# Generate proper license file
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/LICENSE" "${CMAKE_CURRENT_BINARY_DIR}/final-LICENSE.txt")

file(GLOB LICENSE_FILES "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_*.txt")
foreach(LICENSE_FILE ${LICENSE_FILES})
	cat(${LICENSE_FILE} "${CMAKE_CURRENT_BINARY_DIR}/final-LICENSE.txt")
endforeach()

# Install
install (TARGETS xava DESTINATION bin)
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/final-LICENSE.txt" DESTINATION share/licenses/xava)
install (FILES example_files/config RENAME config.example DESTINATION share/xava)
install (FILES example_files/gl/shaders/pre/default/vertex.glsl    RENAME vertex.glsl.example   DESTINATION share/xava/gl/shaders/pre/default)
install (FILES example_files/gl/shaders/pre/default/fragment.glsl  RENAME fragment.glsl.example DESTINATION share/xava/gl/shaders/pre/default)
install (FILES example_files/gl/shaders/post/default/vertex.glsl   RENAME vertex.glsl.example   DESTINATION share/xava/gl/shaders/post/default)
install (FILES example_files/gl/shaders/post/default/fragment.glsl RENAME fragment.glsl.example DESTINATION share/xava/gl/shaders/post/default)
install (FILES example_files/gl/shaders/post/kinetic/vertex.glsl   RENAME vertex.glsl.example   DESTINATION share/xava/gl/shaders/post/kinetic)
install (FILES example_files/gl/shaders/post/kinetic/fragment.glsl RENAME fragment.glsl.example DESTINATION share/xava/gl/shaders/post/kinetic)
if(UNIX AND NOT APPLE)
    install (FILES ${CMAKE_BINARY_DIR}/xava.desktop DESTINATION share/applications)
    install (FILES assets/linux/xava.svg DESTINATION share/icons/hicolor/scalable/apps)
endif()

