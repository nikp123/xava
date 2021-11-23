# Project default
option(CAIRO_MODULES "GL_MODULES" ON)

# Xorg
if(CAIRO_MODULES)
    # OpenGL/CAIRO
    pkg_check_modules(CAIRO QUIET cairo)
    if(CAIRO_FOUND)
        add_library(cairo_bars SHARED "${XAVA_MODULE_DIR}/main.c"
                                    "${GLOBAL_FUNCTION_SOURCES}")
                                #"${XAVA_MODULE_DIR}/../../"
        target_link_directories(cairo_bars PRIVATE 
            "${CAIRO_LIBRARY_DIRS}")
        target_include_directories(cairo_bars PRIVATE 
            "${CAIRO_INCLUDE_DIRS}")
        if(WINDOWS OR MINGW OR MSVC OR CYGWIN)
            target_link_libraries(cairo_bars xava-shared CAIRO)
            target_compile_definitions(cairo_bars PUBLIC -DCAIRO)
        else()
            target_link_libraries(cairo_bars xava-shared "${CAIRO_LIBRARIES}")
            target_compile_definitions(cairo_bars PUBLIC -DCAIRO)
        endif()
        set_target_properties(cairo_bars PROPERTIES PREFIX "")
        set_target_properties(cairo_bars PROPERTIES IMPORT_PREFIX "")
        configure_file("${XAVA_MODULE_DIR}/config.cfg"   cairo/module/bars/config.cfg   COPYONLY)
        set_target_properties(cairo_bars PROPERTIES OUTPUT_NAME "cairo/module/bars/module")
        install(TARGETS cairo_bars RENAME module DESTINATION share/xava/cairo/module/bars/)
    else()
        message(WARNING "CAIRO library not found; \"cairo_bars\" won't build")
    endif()
endif()
