# Wayland protocols default directory
set(WL_PROT_DIR "/usr/share/wayland-protocols")

# Project default
option(WAYLAND "WAYLAND" ON)

function(generate_wayland_source input output)
    get_filename_component(FILE_EXT "${output}" LAST_EXT)
    if(FILE_EXT MATCHES ".c")
        add_custom_command(OUTPUT "${XAVA_MODULE_DIR}/gen/${output}"
            COMMAND wayland-scanner private-code
            "${input}"
            "${XAVA_MODULE_DIR}/gen/${output}"
            VERBATIM)
    elseif(FILE_EXT MATCHES ".h")
        add_custom_command(OUTPUT "${XAVA_MODULE_DIR}/gen/${output}"
            COMMAND wayland-scanner client-header
            "${input}"
            "${XAVA_MODULE_DIR}/gen/${output}"
            VERBATIM)

        if(_EXIT_CODE)
            message(FATAL_ERROR "An error occured while processing ${input}")
        endif()
    else()
        message(FATAL_ERROR "Invalid file extension: ${extension} of file ${output}")
    endif()
endfunction()


# Wayland
if(WAYLAND)
    pkg_check_modules(WAYLAND QUIET wayland-client)
    if(WAYLAND_FOUND)
        set(_CFLAG_SYMBOLS_HIDE "-fvisibility=hidden")

        # Hacky way to deal with wayland-scanner (but will be used for now)
        generate_wayland_source(
            "${WL_PROT_DIR}/stable/xdg-shell/xdg-shell.xml"
            "xdg-shell-client-protocol.h")
        generate_wayland_source(
            "${WL_PROT_DIR}/stable/xdg-shell/xdg-shell.xml"
            "xdg-shell-client-protocol.c")
        generate_wayland_source(
            "${WL_PROT_DIR}/unstable/xdg-output/xdg-output-unstable-v1.xml"
            "xdg-output-unstable-v1-client-protocol.h")
        generate_wayland_source(
            "${WL_PROT_DIR}/unstable/xdg-output/xdg-output-unstable-v1.xml"
            "xdg-output-unstable-v1-client-protocol.c")

        generate_wayland_source(
            "${XAVA_MODULE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml"
            "wlr-layer-shell-unstable-v1-client-protocol.h")
        generate_wayland_source(
            "${XAVA_MODULE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml"
            "wlr-layer-shell-unstable-v1-client-protocol.c")
        generate_wayland_source(
            "${XAVA_MODULE_DIR}/protocols/wlr-output-management-unstable-v1.xml"
            "wlr-output-managment-unstable-v1.h")
        generate_wayland_source(
            "${XAVA_MODULE_DIR}/protocols/wlr-output-management-unstable-v1.xml"
            "wlr-output-managment-unstable-v1.c")

        # add winapi test
        list(APPEND DEFAULT_OUTPUT_SOURCES "${XAVA_MODULE_DIR}/test.c")
        list(APPEND DEFAULT_OUTPUT_LINKDIR "${WAYLAND_LIBRARY_DIRS}")
        list(APPEND DEFAULT_OUTPUT_LINKLIB "${WAYLAND_LIBRARIES}")
        list(APPEND DEFAULT_OUTPUT_INCDIR  "${WAYLAND_INCLUDE_DIRS}")
        list(APPEND DEFAULT_OUTPUT_DEF     "-DWAYLAND")

        # Wayland is such a fucking mess that I won't even bother adding license disclaimers
        # Only if someone complains, then I might
    else()
        message(WARNING "Wayland libraries not found, Wayland won't build")
    endif()

    pkg_check_modules(WAYLAND_EGL QUIET egl wayland-egl)
    if(WAYLAND_FOUND AND WAYLAND_EGL_FOUND)
        add_library(out_wayland_opengl SHARED
            "${XAVA_MODULE_DIR}/main.c"
            "${XAVA_MODULE_DIR}/wl_output.c"
            "${XAVA_MODULE_DIR}/registry.c"
            "${XAVA_MODULE_DIR}/xdg.c"
            "${XAVA_MODULE_DIR}/egl.c"
            "${XAVA_MODULE_DIR}/zwlr.c"
            "src/output/shared/gl/egl.c"
            "src/output/shared/gl/main.c"
            "src/output/shared/graphical.c"
            "${XAVA_MODULE_DIR}/gen/xdg-shell-client-protocol.c"
            "${XAVA_MODULE_DIR}/gen/xdg-output-unstable-v1-client-protocol.c"
            "${XAVA_MODULE_DIR}/gen/wlr-output-managment-unstable-v1.c"
            "${XAVA_MODULE_DIR}/gen/wlr-layer-shell-unstable-v1-client-protocol.c"
            "${GLOBAL_FUNCTION_SOURCES}")
        target_link_libraries(out_wayland_opengl xava-shared
            ${WAYLAND_LIBRARIES} ${WAYLAND_EGL_LIBRARIES} OpenGL
            GL wayland-egl GLEW)
        target_include_directories(out_wayland_opengl PRIVATE
            ${WAYLAND_INCLUDE_DIRS} ${WAYLAND_EGL_INCLUDE_DIRS})
        target_link_directories(out_wayland_opengl PRIVATE
            ${WAYLAND_LIBRARY_DIRS} ${WAYLAND_EGL_LIBRARY_DIRS})
        set_target_properties(out_wayland_opengl PROPERTIES PREFIX "")
        install(TARGETS out_wayland_opengl DESTINATION lib/xava)
        target_compile_definitions(out_wayland_opengl PUBLIC -DWAYLAND -DEGL)
    else()
        message(WARNING "Wayland EGL libraries not found, \"wayland\" won't build")
    endif()

    pkg_check_modules(CAIRO QUIET cairo)
    if(WAYLAND_FOUND AND CAIRO_FOUND)
        add_library(out_wayland_cairo SHARED
            "${XAVA_MODULE_DIR}/main.c"
            "${XAVA_MODULE_DIR}/wl_output.c"
            "${XAVA_MODULE_DIR}/registry.c"
            "${XAVA_MODULE_DIR}/xdg.c"
            "${XAVA_MODULE_DIR}/cairo.c"
            "${XAVA_MODULE_DIR}/shm.c"
            "${XAVA_MODULE_DIR}/zwlr.c"
            "src/output/shared/cairo/main.c"
            "src/output/shared/cairo/util/feature_compat.c"
            "src/output/shared/cairo/util/module.c"
            "src/output/shared/cairo/util/region.c"
            "src/output/shared/graphical.c"
            "${XAVA_MODULE_DIR}/gen/xdg-shell-client-protocol.c"
            "${XAVA_MODULE_DIR}/gen/xdg-output-unstable-v1-client-protocol.c"
            "${XAVA_MODULE_DIR}/gen/wlr-output-managment-unstable-v1.c"
            "${XAVA_MODULE_DIR}/gen/wlr-layer-shell-unstable-v1-client-protocol.c"
            "${GLOBAL_FUNCTION_SOURCES}")
        target_link_libraries(out_wayland_cairo xava-shared
            ${WAYLAND_LIBRARIES} ${CAIRO_LIBRARIES})
        target_include_directories(out_wayland_cairo PRIVATE
            ${WAYLAND_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS})
        target_link_directories(out_wayland_cairo PRIVATE
            ${WAYLAND_LIBRARY_DIRS} ${CAIRO_LIBRARY_DIRS})
        set_target_properties(out_wayland_cairo PROPERTIES PREFIX "")
        install(TARGETS out_wayland_cairo DESTINATION lib/xava)
        target_compile_definitions(out_wayland_cairo PUBLIC -DWAYLAND -DCAIRO
            -DSHM)
    else()
        message(WARNING "Wayland Cairo libraries not found, \"wayland_cairo\" won't build")
    endif()
endif()
