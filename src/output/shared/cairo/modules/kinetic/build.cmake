# Project default
option(CAIRO_MODULES "GL_MODULES" ON)

# Xorg
if(CAIRO_MODULES)
    # OpenGL/CAIRO
    pkg_check_modules(CAIRO QUIET cairo)
    if(CAIRO_FOUND)
        add_library(cairo_kinetic SHARED "${XAVA_MODULE_DIR}/main.c"
                                    "${GLOBAL_FUNCTION_SOURCES}")
                                #"${XAVA_MODULE_DIR}/../../"
        target_link_directories(cairo_kinetic PRIVATE
            "${CAIRO_LIBRARY_DIRS}")
        target_include_directories(cairo_kinetic PRIVATE
            "${CAIRO_INCLUDE_DIRS}")
        if(WINDOWS OR MINGW OR MSVC OR CYGWIN)
            target_link_libraries(cairo_kinetic xava-shared CAIRO)
            target_compile_definitions(cairo_kinetic PUBLIC -DCAIRO)
        else()
            target_link_libraries(cairo_kinetic xava-shared "${CAIRO_LIBRARIES}")
            target_compile_definitions(cairo_kinetic PUBLIC -DCAIRO)
        endif()
        set_target_properties(cairo_kinetic PROPERTIES PREFIX "")
        set_target_properties(cairo_kinetic PROPERTIES IMPORT_PREFIX "")
        configure_file("${XAVA_MODULE_DIR}/config.cfg"   cairo/module/kinetic/config.cfg   COPYONLY)
        set_target_properties(cairo_kinetic PROPERTIES OUTPUT_NAME "cairo/module/kinetic/module")
        install(TARGETS cairo_kinetic RENAME module DESTINATION share/xava/cairo/module/kinetic/)
    else()
        message(WARNING "CAIRO library not found; \"cairo_kinetic\" won't build")
    endif()
endif()
