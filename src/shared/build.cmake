project(xava-shared C)

# Optional features
set(ADDITIONAL_SHARED_SOURCES "")
set(ADDITIONAL_SHARED_LIBRARIES "")
set(ADDITIONAL_SHARED_DEFINITIONS "")
set(ADDITIONAL_SHARED_INCLUDE_DIRS "")

# Pull git required submodules
execute_process(COMMAND git submodule update --init)

# Pull submodule and install dependency
add_library(iniparser STATIC
		lib/iniparser/src/dictionary.c
		lib/iniparser/src/iniparser.c)
set_target_properties(iniparser PROPERTIES COMPILE_FLAGS "-fPIC")

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

set(EFSW_INSTALL OFF)
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(lib/efsw)

# Build XAVA shared library
add_library(xava-shared SHARED
	src/shared/log.c
	src/shared/config/config.c
	src/shared/module/abstractions.c
	src/shared/ionotify.cpp
	src/shared/io.c
	${ADDITIONAL_SHARED_SOURCES}
)
set_target_properties(xava-shared PROPERTIES COMPILE_FLAGS "-fPIC")
target_link_libraries(xava-shared PRIVATE ${ADDITIONAL_SHARED_LIBRARIES} iniparser efsw)
target_compile_definitions(xava-shared PRIVATE ${ADDITIONAL_SHARED_DEFINITIONS})
target_include_directories(xava-shared PRIVATE "${ADDITIONAL_SHARED_INCLUDE_DIRS}" lib/iniparser/src lib/efsw/include)

install (TARGETS xava-shared DESTINATION lib)

