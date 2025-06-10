# Project default
option(CAIRO_MODULES "CAIRO_MODULES" ON)

if(CAIRO_MODULES)
    pkg_check_modules(CAIRO QUIET cairo)
    if(CAIRO_FOUND)
        add_library(cairo_oscilloscope SHARED "${XAVA_MODULE_DIR}/main.c"
                                    "${XAVA_MODULE_DIR}/../shared/config.c"
                                    "${GLOBAL_FUNCTION_SOURCES}")

        target_link_directories(cairo_oscilloscope PRIVATE
            "${CAIRO_LIBRARY_DIRS}")
        target_include_directories(cairo_oscilloscope PRIVATE
            "${CAIRO_INCLUDE_DIRS}")
        target_link_libraries(cairo_oscilloscope xava-shared "${CAIRO_LIBRARIES}")

        target_compile_definitions(cairo_oscilloscope PUBLIC -DCAIRO)
        set_target_properties(cairo_oscilloscope PROPERTIES PREFIX "")
        set_target_properties(cairo_oscilloscope PROPERTIES IMPORT_PREFIX "")
        configure_file("${XAVA_MODULE_DIR}/config.ini"   cairo/module/oscilloscope/config.ini   COPYONLY)

        # this copies the dlls for mr. windows
        #find_and_copy_dlls(cairo_oscilloscope)

        set_target_properties(cairo_oscilloscope PROPERTIES OUTPUT_NAME "cairo/module/oscilloscope/module")
        install(FILES "${CMAKE_BINARY_DIR}/cairo/module/oscilloscope/config.ini" DESTINATION share/xava/cairo/module/oscilloscope/)
        install(TARGETS cairo_oscilloscope RENAME module DESTINATION share/xava/cairo/module/oscilloscope/)
    else()
        message(WARNING "CAIRO library not found; \"cairo_oscilloscope\" won't build")
    endif()
endif()
