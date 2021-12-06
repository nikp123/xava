# Project default
option(CAIRO_MODULES "CAIRO_MODULES" ON)

if(CAIRO_MODULES)
    pkg_check_modules(CAIRO QUIET cairo)
    if(CAIRO_FOUND)
        add_library(cairo_bars SHARED "${XAVA_MODULE_DIR}/main.c"
                                    "${GLOBAL_FUNCTION_SOURCES}")
                                #"${XAVA_MODULE_DIR}/../../"

        target_link_directories(cairo_bars PRIVATE
            "${CAIRO_LIBRARY_DIRS}")
        target_include_directories(cairo_bars PRIVATE
            "${CAIRO_INCLUDE_DIRS}")
        target_link_libraries(cairo_bars xava-shared "${CAIRO_LIBRARIES}")

        target_compile_definitions(cairo_bars PUBLIC -DCAIRO)
        set_target_properties(cairo_bars PROPERTIES PREFIX "")
        set_target_properties(cairo_bars PROPERTIES IMPORT_PREFIX "")
        configure_file("${XAVA_MODULE_DIR}/config.ini"   cairo/module/bars/config.ini   COPYONLY)

        # this copies the dlls for mr. windows
        #if(MINGW)
        #    add_custom_command(TARGET cairo_bars POST_BUILD
        #        COMMAND ${CMAKE_COMMAND} -E env MINGW_BUNDLEDLLS_SEARCH_PATH="./:${xava_dep_dirs}"
        #        python "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/mingw-bundledlls/mingw-bundledlls" $<TARGET_FILE:cairo_bars> --copy
        #    )
        #endif()

        set_target_properties(cairo_bars PROPERTIES OUTPUT_NAME "cairo/module/bars/module")
        install(TARGETS cairo_bars RENAME module DESTINATION share/xava/cairo/module/bars/)
    else()
        message(WARNING "CAIRO library not found; \"cairo_bars\" won't build")
    endif()
endif()
