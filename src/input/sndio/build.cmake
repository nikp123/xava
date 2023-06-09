# The project default
option(SNDIO "SNDIO" ON)

# sndio
if(SNDIO)
    find_library(SNDIO_LIB sndio HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
    if(SNDIO_LIB)
        add_definitions(-DSNDIO)
        add_library(in_sndio SHARED "${XAVA_MODULE_DIR}/main.c"
                                    "${GLOBAL_FUNCTION_SOURCES}")
        target_link_libraries(in_sndio xava-shared "-lsndio")
        set_target_properties(in_sndio PROPERTIES PREFIX "")
        install(TARGETS in_sndio DESTINATION lib/xava)

        # Add legal disclaimer
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_sndio.txt"
            "SNDIO license can be obtained at: https://en.wikipedia.org/wiki/ISC_license#License_terms on behalf of Alexandre Ratchov (C) 2013\n")
    else()
        message(WARNING "SNDIO library missing; SNDIO won't build")
    endif()
endif()

