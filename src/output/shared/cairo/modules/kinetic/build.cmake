# Project default
option(CAIRO_MODULES "CAIRO_MODULES" ON)

if(CAIRO_MODULES)
    pkg_check_modules(CAIRO QUIET cairo)
    if(CAIRO_FOUND)
        add_library(cairo_kinetic SHARED "${XAVA_MODULE_DIR}/main.c"
                                    "${GLOBAL_FUNCTION_SOURCES}")
                                #"${XAVA_MODULE_DIR}/../../"

        target_link_directories(cairo_kinetic PRIVATE
            "${CAIRO_LIBRARY_DIRS}")
        target_include_directories(cairo_kinetic PRIVATE
            "${CAIRO_INCLUDE_DIRS}")
        target_link_libraries(cairo_kinetic xava-shared "${CAIRO_LIBRARIES}")

        target_compile_definitions(cairo_kinetic PUBLIC -DCAIRO)
        set_target_properties(cairo_kinetic PROPERTIES PREFIX "")
        set_target_properties(cairo_kinetic PROPERTIES IMPORT_PREFIX "")
        configure_file("${XAVA_MODULE_DIR}/config.ini"   cairo/module/kinetic/config.ini   COPYONLY)

        # this copies the dlls for mr. windows
        #if(MINGW)
        #    string(JOIN ":" xava_dep_dirs ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES} ${CMAKE_FIND_ROOT_PATH}/bin)
        #    add_custom_command(TARGET cairo_kinetic POST_BUILD
        #        COMMAND ${CMAKE_COMMAND} -E env MINGW_BUNDLEDLLS_SEARCH_PATH="./:${xava_dep_dirs}"
        #        python "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/mingw-bundledlls/mingw-bundledlls" $<TARGET_FILE:cairo_kinetic> --copy
        #    )
        #endif()

        set_target_properties(cairo_kinetic PROPERTIES OUTPUT_NAME "cairo/module/kinetic/module")
        install(TARGETS cairo_kinetic RENAME module DESTINATION share/xava/cairo/module/kinetic/)
    else()
        message(WARNING "CAIRO library not found; \"cairo_kinetic\" won't build")
    endif()
endif()
