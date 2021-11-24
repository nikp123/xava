# Wayland protocols default directory
set(WL_PROT_DIR "/usr/share/wayland-protocols")

# Project default
option(WAYLAND "WAYLAND" ON)

# Wayland
if(WAYLAND)
    pkg_check_modules(WAYLAND QUIET wayland-client)
    if(WAYLAND_FOUND)
        set(_CFLAG_SYMBOLS_HIDE "-fvisibility=hidden")

        # Hacky way to deal with wayland-scanner (but will be used for now)
        execute_process(COMMAND wayland-scanner client-header
            "${WL_PROT_DIR}/stable/xdg-shell/xdg-shell.xml"
            "${XAVA_MODULE_DIR}/gen/xdg-shell-client-protocol.h")
        execute_process(COMMAND wayland-scanner private-code
            "${WL_PROT_DIR}/stable/xdg-shell/xdg-shell.xml"
            "${XAVA_MODULE_DIR}/gen/xdg-shell-client-protocol.c")
        execute_process(COMMAND wayland-scanner client-header
            "${WL_PROT_DIR}/unstable/xdg-output/xdg-output-unstable-v1.xml"
            "${XAVA_MODULE_DIR}/gen/xdg-output-unstable-v1-client-protocol.h")
        execute_process(COMMAND wayland-scanner private-code
            "${WL_PROT_DIR}/unstable/xdg-output/xdg-output-unstable-v1.xml"
            "${XAVA_MODULE_DIR}/gen/xdg-output-unstable-v1-client-protocol.c")

        execute_process(COMMAND wayland-scanner client-header
            "${XAVA_MODULE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml"
            "${XAVA_MODULE_DIR}/gen/wlr-layer-shell-unstable-v1-client-protocol.h")
        execute_process(COMMAND wayland-scanner private-code
            "${XAVA_MODULE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml"
            "${XAVA_MODULE_DIR}/gen/wlr-layer-shell-unstable-v1-client-protocol.c")
        execute_process(COMMAND wayland-scanner client-header
            "${XAVA_MODULE_DIR}/protocols/wlr-output-management-unstable-v1.xml"
            "${XAVA_MODULE_DIR}/gen/wlr-output-managment-unstable-v1.h")
        execute_process(COMMAND wayland-scanner private-code
            "${XAVA_MODULE_DIR}/protocols/wlr-output-management-unstable-v1.xml"
            "${XAVA_MODULE_DIR}/gen/wlr-output-managment-unstable-v1.c")

        # Wayland is such a fucking mess that I won't even bother adding license disclaimers
        # Only if someone complains, then I might
    else()
        message(WARNING "Wayland libraries not found, Wayland won't build")
    endif()

    pkg_check_modules(WAYLAND_EGL QUIET egl wayland-egl)
    if(WAYLAND_FOUND AND WAYLAND_EGL_FOUND)
        add_library(out_wayland SHARED
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
        target_link_libraries(out_wayland xava-shared 
            ${WAYLAND_LIBRARIES} ${WAYLAND_EGL_LIBRARIES} OpenGL
            GL wayland-egl GLEW)
        target_include_directories(out_wayland PRIVATE 
            ${WAYLAND_INCLUDE_DIRS} ${WAYLAND_EGL_INCLUDE_DIRS})
        target_link_directories(out_wayland PRIVATE 
            ${WAYLAND_LIBRARY_DIRS} ${WAYLAND_EGL_LIBRARY_DIRS})
        set_target_properties(out_wayland PROPERTIES PREFIX "")
        install(TARGETS out_wayland DESTINATION lib/xava)
        target_compile_definitions(out_wayland PUBLIC -DWAYLAND -DEGL)
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
