# Project default
option(CAIRO_MODULES "CAIRO_MODULES" ON)

if(CAIRO_MODULES)
    pkg_check_modules(CAIRO QUIET cairo)
    if(CAIRO_FOUND)
        add_library(cairo_stars SHARED "${XAVA_MODULE_DIR}/main.c"
                                    "${XAVA_MODULE_DIR}/../shared/config.c"
                                    "${GLOBAL_FUNCTION_SOURCES}")
        target_link_directories(cairo_stars PRIVATE 
            "${CAIRO_LIBRARY_DIRS}")
        target_include_directories(cairo_stars PRIVATE 
            "${CAIRO_INCLUDE_DIRS}")
        if(WINDOWS OR MINGW OR MSVC OR CYGWIN)
            target_link_libraries(cairo_stars xava-shared CAIRO)
            target_compile_definitions(cairo_stars PUBLIC -DCAIRO)
        else()
            target_link_libraries(cairo_stars xava-shared "${CAIRO_LIBRARIES}")
            target_compile_definitions(cairo_stars PUBLIC -DCAIRO)
        endif()
        set_target_properties(cairo_stars PROPERTIES PREFIX "")
        set_target_properties(cairo_stars PROPERTIES IMPORT_PREFIX "")
        configure_file("${XAVA_MODULE_DIR}/config.ini" cairo/module/stars/config.ini COPYONLY)
        set_target_properties(cairo_stars PROPERTIES OUTPUT_NAME "cairo/module/stars/module")
        install(TARGETS gl_bars_circle RENAME module DESTINATION share/xava/gl/module/bars_circle/)
        install(FILES "${CMAKE_BINARY_DIR}/cairo/module/stars/config.ini" RENAME config.ini.example DESTINATION share/xava/cairo/module/stars/)
        install(TARGETS cairo_stars RENAME module DESTINATION share/xava/cairo/module/stars/)
    else()
        message(WARNING "CAIRO library not found; \"cairo_stars\" won't build")
    endif()
endif()
