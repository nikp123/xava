project(xava-shared C)

# Optional features
set(ADDITIONAL_SHARED_SOURCES "")
set(ADDITIONAL_SHARED_LIBRARIES "")
set(ADDITIONAL_SHARED_DEFINITIONS "")
set(ADDITIONAL_SHARED_INCLUDE_DIRS "")

# Build (or include) shared dependency - iniparser
find_library(INIPARSER iniparser HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
if(NOT INIPARSER)
	message(STATUS "iniparser not found on system, building from source.")

	# Pull submodule and install dependency
	execute_process(COMMAND git submodule update --init)
	add_library(iniparser SHARED
			lib/iniparser/src/dictionary.c
			lib/iniparser/src/iniparser.c)
	list(APPEND ADDITIONAL_SHARED_DEFINITIONS "-DINIPARSER")
else()
	# certain distros like ubuntu put iniparser in a subdirectory "iniparser"
	# this is just a non-destructive way to accomidate that case

	find_file(INIPARSER_INCLUDE_FILE iniparser/iniparser.h ${CMAKE_SYSTEM_INCLUDE_PATH})
	if(NOT ${INIPARSER_INCLUDE_FILE} STREQUAL "INIPARSER_INCLUDE_FILE-NOTFOUND")
		string(REGEX REPLACE "iniparser.h" "" INIPARSER_INCLUDE_DIR ${INIPARSER_INCLUDE_FILE})
		list(APPEND ADDITIONAL_SHARED_INCLUDE_DIRS ${INIPARSER_INCLUDE_DIR})
	endif()
endif()

if(DEFINE_LEGACYINIPARSER AND INIPARSER)
	list(APPEND ADDITIONAL_SHARED_DEFINITIONS "-DLEGACYINIPARSER")
endif()


# IO notify part (this check is supposed to be Linux, but CMake is designed by fucking toddlers)
if(UNIX AND NOT APPLE)
	list(APPEND ADDITIONAL_SHARED_SOURCES "src/shared/ionotify/linux.c")
elseif(MSYS OR MINGW OR MSVC)
	list(APPEND ADDITIONAL_SHARED_SOURCES "src/shared/ionotify/windows.c")
else()
	message(WARNING "IO notify operations not supported on current platform")
endif()

# Runtime library load
if(UNIX)
	list(APPEND ADDITIONAL_SHARED_LIBRARIES "dl")
	list(APPEND ADDITIONAL_SHARED_SOURCES "src/shared/module/unix.c")
elseif(MSYS OR MINGW OR MSVC)
	list(APPEND ADDITIONAL_SHARED_SOURCES "src/shared/module/windows.c")
else()
	message(FATAL_ERROR "Your OS/Platform does NOT support modules!")
endif()

#target_link_libraries(out_wayland_egl xava-shared "${WAYLAND_LIBRARIES}" EGL GLESv2 wayland-egl)
#target_link_directories(out_wayland_egl PRIVATE "${WAYLAND_LIBRARY_DIRS}")
#set_target_properties(out_wayland_egl PROPERTIES PREFIX "")

# Build XAVA shared library
add_library(xava-shared SHARED
	src/shared/log.c
	src/shared/config/config.c
	src/shared/module/abstractions.c
	src/shared/ionotify/abstractions.c
	src/shared/io.c
	${ADDITIONAL_SHARED_SOURCES}
)
target_link_libraries(xava-shared ${ADDITIONAL_SHARED_LIBRARIES} iniparser)
target_compile_definitions(xava-shared PRIVATE ${ADDITIONAL_SHARED_DEFINITIONS})
target_include_directories(xava-shared PRIVATE "${ADDITIONAL_SHARED_INCLUDE_DIRS}")

install (TARGETS xava-shared DESTINATION lib)

