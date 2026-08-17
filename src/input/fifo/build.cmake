# Project default
option(FIFO "FIFO" ON)

# Doesn't work on Windows (no shit)
if(MSYS OR MINGW OR MSVC)
    set(FIFO OFF)
endif()

if(FIFO)
    message(STATUS "Not a Windows platform, can use POSIX now!")
    add_library(in_fifo SHARED "${XAVA_MODULE_DIR}/main.c"
                                "${GLOBAL_FUNCTION_SOURCES}")
    target_link_libraries(in_fifo xava-shared)
    set_target_properties(in_fifo PROPERTIES
        # Strip prefix from the resulting library
        PREFIX ""
        # Set RPATH (where to look for system dependencies)
        INSTALL_RPATH "$ORIGIN:$ORIGIN/.."
        # Force RPATH
        LINK_FLAGS "-Wl,--disable-new-dtags"
    )
    install(TARGETS in_fifo DESTINATION lib/xava)
endif()

