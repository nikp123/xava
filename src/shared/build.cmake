project(xava-shared C)

# Optional features
set(ADDITIONAL_SHARED_SOURCES "")
set(ADDITIONAL_SHARED_LIBRARIES "")
set(ADDITIONAL_SHARED_DEFINITIONS "")
set(ADDITIONAL_SHARED_INCLUDE_DIRS "")

# Pull git required submodules
if(NOT NIX_BUILDER)
    execute_process(COMMAND git submodule update --init)
endif()

# Pull submodule and install dependency
add_library(iniparser STATIC
        lib/iniparser/src/dictionary.c
        lib/iniparser/src/iniparser.c)
set_target_properties(iniparser PROPERTIES COMPILE_FLAGS "-fPIC")

pkg_check_modules(DEPS QUIET libcurl taglib zlib dbus-1)
if(DEPS_FOUND)
    add_subdirectory(lib/kiss-mpris-properties)

    # Runtime library load
    if(UNIX)
        list(APPEND ADDITIONAL_SHARED_LIBRARIES "dl")
        list(APPEND ADDITIONAL_SHARED_SOURCES "src/shared/module/unix.c")
        list(APPEND ADDITIONAL_SHARED_SOURCES "src/shared/io/unix.c")
    elseif(MSYS OR MINGW OR MSVC)
        list(APPEND ADDITIONAL_SHARED_SOURCES "src/shared/module/windows.c")
    else()
        message(FATAL_ERROR "Your OS/Platform does NOT support modules!")
    endif()

    # Build XAVA shared library
    add_library(xava-shared SHARED
        src/shared/config/config.c
        src/shared/config/pywal.c
        src/shared/genre.c
        src/shared/ionotify.c
        src/shared/io/io.c
        src/shared/log.c
        src/shared/media_info/artwork.c
        src/shared/media_info/artwork_taglib.cpp
        src/shared/media_info/media_info.c
        src/shared/module/abstractions.c
        src/shared/util/version.c
        ${ADDITIONAL_SHARED_SOURCES}
    )
    set_target_properties(xava-shared PROPERTIES COMPILE_FLAGS "-fPIC")
    target_link_directories(xava-shared PRIVATE
        "${DEPS_LIBRARY_DIRS}")
    target_link_libraries(xava-shared PRIVATE ${ADDITIONAL_SHARED_LIBRARIES}
        iniparser pthread kiss-mpris ${DEPS_LIBRARIES})
    target_compile_definitions(xava-shared PRIVATE ${ADDITIONAL_SHARED_DEFINITIONS})
    target_include_directories(xava-shared PRIVATE
        "${ADDITIONAL_SHARED_INCLUDE_DIRS}"
        "${DEPS_INCLUDE_DIRS}"
        lib/iniparser/src
        lib/kiss-mpris-properties/src
        lib/stb
        lib/x-watcher
    )

    install(TARGETS xava-shared DESTINATION lib)

    find_and_copy_dlls(xava-shared)

    # Test the library because it's very buggy
    add_executable(basic_event_stack src/shared/test/basic_event_stack.c)
    target_link_libraries(basic_event_stack PRIVATE xava-shared)
    add_test(NAME basic_event_stack COMMAND basic_event_stack)
else()
    message(FATAL_ERROR "TagLib, Zlib, DBus-1 and/or CURL library not found; XAVA cannot build!")
endif()

# Add legal disclaimers
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_iniparser.txt"
    "INIparser license can be obtained at: https://raw.githubusercontent.com/ndevilla/iniparser/master/LICENSE\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_xwatcher.txt"
    "xWatcher license can be obtained at: https://raw.githubusercontent.com/nikp123/x-watcher/master/README.md\n")

