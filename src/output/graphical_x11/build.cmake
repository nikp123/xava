# Project default
option(X11 "X11" ON)

# Xorg
if(X11)
    if(APPLE)
        message(WARNING "Mate, you Apples need to use the legacy thing now.")
    else()
        pkg_check_modules(X11 x11 xfixes xrender xrandr xext)
        if(X11_FOUND)
            # add winapi test
            list(APPEND DEFAULT_OUTPUT_SOURCES "${XAVA_MODULE_DIR}/test.c")
            list(APPEND DEFAULT_OUTPUT_LINKDIR "${X11_LIBRARY_DIRS}")
            list(APPEND DEFAULT_OUTPUT_LINKLIB "${X11_LIBRARIES}")
            list(APPEND DEFAULT_OUTPUT_INCDIR  "${X11_INCLUDE_DIRS}")
            list(APPEND DEFAULT_OUTPUT_DEF     "-DX11")

            # OpenGL/GLX
            pkg_check_modules(GLX QUIET gl glew)
            if(GLX_FOUND)
                add_library(out_x11_opengl SHARED "${XAVA_MODULE_DIR}/main.c"
                                            "src/output/shared/graphical.c"
                                            "src/output/shared/gl/glew.c"
                                            "src/output/shared/gl/main.c"
                                            "${GLOBAL_FUNCTION_SOURCES}")
                target_link_directories(out_x11_opengl PRIVATE
                    "${GLX_LIBRARY_DIRS}" "${X11_LIBRARY_DIRS}")
                target_include_directories(out_x11_opengl PRIVATE
                    "${GLX_INCLUDE_DIRS}" "${X11_INCLUDE_DIRS}")
                target_link_libraries(out_x11_opengl xava-shared
                    "${GLX_LIBRARIES}" "${X11_LIBRARIES}")
                target_compile_definitions(out_x11_opengl PUBLIC -DGL)
                set_target_properties(out_x11_opengl PROPERTIES PREFIX "")
                install(TARGETS out_x11_opengl DESTINATION lib/xava)

                # Maybe GL license?
            else()
                message(WARNING "GLEW and/or OpenGL library not found; \"x11_gl\" won't build")
            endif()

            # EGL
            pkg_check_modules(EGL QUIET egl glesv2 glew)
            if(EGL_FOUND)
                add_library(out_x11_egl SHARED "${XAVA_MODULE_DIR}/main.c"
                                            "src/output/shared/graphical.c"
                                            "src/output/shared/gl/egl.c"
                                            "src/output/shared/gl/main.c"
                                            "${GLOBAL_FUNCTION_SOURCES}")
                target_link_directories(out_x11_egl PRIVATE
                    "${EGL_LIBRARY_DIRS}" "${X11_LIBRARY_DIRS}")
                target_include_directories(out_x11_egl PRIVATE
                    "${EGL_INCLUDE_DIRS}" "${X11_INCLUDE_DIRS}")
                target_link_libraries(out_x11_egl xava-shared GLEW
                    "${EGL_LIBRARIES}" "${X11_LIBRARIES}")
                target_compile_definitions(out_x11_egl PUBLIC -DEGL)
                set_target_properties(out_x11_egl PROPERTIES PREFIX "")
                install(TARGETS out_x11_egl DESTINATION lib/xava)

                # Maybe EGL license?
            else()
              message(WARNING "GLEW EGL and/or GLESv2 library not found; \"x11_egl\" won't build")
            endif()

            pkg_check_modules(CAIRO QUIET cairo)
            if(CAIRO_FOUND)
                add_library(out_x11_cairo SHARED "${XAVA_MODULE_DIR}/main.c"
                                            "src/output/shared/graphical.c"
                                            "src/output/shared/cairo/main.c"
                                            "src/output/shared/cairo/util/module.c"
                                            "src/output/shared/cairo/util/feature_compat.c"
                                            "src/output/shared/cairo/util/region.c"
                                            "${GLOBAL_FUNCTION_SOURCES}")
                target_link_directories(out_x11_cairo PRIVATE
                    "${CAIRO_LIBRARY_DIRS}" "${X11_LIBRARY_DIRS}")
                target_include_directories(out_x11_cairo PRIVATE
                    "${CAIRO_INCLUDE_DIRS}" "${X11_INCLUDE_DIRS}")
                target_link_libraries(out_x11_cairo xava-shared
                    "${CAIRO_LIBRARIES}" "${X11_LIBRARIES}")
                target_compile_definitions(out_x11_cairo PUBLIC -DCAIRO)
                set_target_properties(out_x11_cairo PROPERTIES PREFIX "")
                install(TARGETS out_x11_cairo DESTINATION lib/xava)

                # Maybe EGL license?
            else()
                message(WARNING "Cairo library not found; \"x11_cairo\" won't build")
            endif()

            # Add legal disclaimer
            file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_x11.txt"
                "X11 license can be obtained at: https://raw.githubusercontent.com/mirror/libX11/master/COPYING\n")
        else()
            message(WARNING "x11, xfixes, xrender, xrandr and/or xext libraries missing; X11 won't build")
        endif()
    endif()
endif()
