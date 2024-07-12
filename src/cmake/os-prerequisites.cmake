# Windows-y things
if(MINGW)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .dll ${CMAKE_FIND_LIBRARY_SUFFIXES})
    add_definitions(-DXAVA_DEFAULT_INPUT="wasapi")

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Since release build, console is being disabled")
        SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
    else()
        set(CMAKE_SUPPORT_WINDOWS_EXPORT_ALL_SYMBOLS 1)
    endif()

    # Do not convert the file paths to Windows-native ones as we're building in Linux
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/example_files" xava_EXAMPLE_FILES)
    string(REPLACE "/" "\\" xava_EXAMPLE_FILES_NSIS "${CMAKE_CURRENT_SOURCE_DIR}/example_files")
    #string(REPLACE "/" "\\" xava_SOURCE_DIR    "${CMAKE_CURRENT_SOURCE_DIR}")

    # Prepare NSI file for compilation
    configure_file("assets/windows/xava.nsi.template" "xava.nsi" NEWLINE_STYLE CRLF)

    set(xava_EXECUTABLE_WINDOWS_ICO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/windows/xava.ico")
    configure_file("assets/windows/xava.rc.template" "xava.rc" NEWLINE_STYLE CRLF)

    set(ADDITIONAL_OBJS ${CMAKE_CURRENT_BINARY_DIR}/xava.rc)

    # For Windows you use ';', but it's broken, so don't even bother
    # If you really care about building this on Windows, learn how to use
    # Docker and build the .github/actions/windows/build.sh file using
    # the windows-archlinux-mingw64-ownstuff image in the repo
    set(xava_dep_dirs_separator ":")

    # set global binary library search path
    string(JOIN "${xava_dep_dirs_separator}" xava_dep_dirs
        ./ ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES} ${CMAKE_FIND_ROOT_PATH}/bin
        /mingw64/bin /mingw32/bin /clang64/bin /clang32/bin /clangarm64/bin)

    add_definitions("-DXAVA_PREDEFINED_SENS_VALUE=0.005")
else()
    # Drop everything besides the path within the project
    # inside of __FILENAME__ references which I use for logging
    #
    # See: https://stackoverflow.com/a/16658858
    #
    # Needed for achieving purity in a nix build
    #
    # Assume we're running this from the ./build directory 
    # so that GDB picks up our source files when debugging
    add_compile_options(-ffile-prefix-map=${CMAKE_SOURCE_DIR}/=../)

    if(GNU)
        set(BUILD_DEBUG_FLAGS "-rdynamic")
        set(CMAKE_SHARED_LINKER_FLAGS "-rdynamic")
    endif()
endif()

