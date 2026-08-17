# Project default
option(SHMEM "SHMEM" OFF)

if(SHMEM)
    add_definitions(-DSHMEM)
    add_library(in_shmem SHARED "${XAVA_MODULE_DIR}/main.c"
                                "${GLOBAL_FUNCTION_SOURCES}")
    target_link_libraries(in_shmem xava-shared "-lrt")
    set_target_properties(in_shmem PROPERTIES
        # Strip prefix from the resulting library
        PREFIX ""
        # Set RPATH (where to look for system dependencies)
        INSTALL_RPATH "$ORIGIN:$ORIGIN/.."
        # Force RPATH
        LINK_FLAGS "-Wl,--disable-new-dtags"
    )
    install(TARGETS in_shmem DESTINATION lib/xava)

    # Maybe license? pls no sue
endif()

