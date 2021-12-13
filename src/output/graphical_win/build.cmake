# The project default
option(WINAPI "WINAPI" ON)

if(NOT (MSYS OR MINGW OR MSVC))
    set(WINAPI OFF)
endif()

# winapi
if(WINAPI)
    find_library(GDI_LIB gdi32 HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
    if(GDI_LIB)
        find_library(GLU_LIB glu32 HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
        if(GLU_LIB)
            find_library(DWM_LIB dwmapi HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
            if(DWM_LIB)
                find_library(WGL_LIB opengl32 HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})

                # This is the only way it works under Windows+CMake for some reason
                find_library(GLEW glew32)

                # add winapi test
                list(APPEND DEFAULT_OUTPUT_SOURCES "${XAVA_MODULE_DIR}/test.c")
                #list(APPEND DEFAULT_OUTPUT_LINKDIR "${WIN_LIBRARY_DIRS}")
                #list(APPEND DEFAULT_OUTPUT_LINKLIB "${WIN_LIBRARIES}")
                #list(APPEND DEFAULT_OUTPUT_INCDIR  "${WIN_INCLUDE_DIRS}")
                list(APPEND DEFAULT_OUTPUT_DEF     "-DWINDOWS")

                if(WGL_LIB AND GLEW)
                    add_library(out_win_opengl SHARED "${XAVA_MODULE_DIR}/main.c"
                        "src/output/shared/graphical.c"
                        "src/output/shared/gl/glew.c"
                        "src/output/shared/gl/main.c"
                        "${GLOBAL_FUNCTION_SOURCES}")
                    target_link_libraries(out_win_opengl xava-shared
                        "-lglew32 -lgdi32 -lwinmm -lopengl32 -ldwmapi")
                    target_compile_definitions(out_win_opengl PUBLIC -DWIN -DGL)
                    set_target_properties(out_win_opengl PROPERTIES PREFIX "")

                    find_and_copy_dlls(out_win_opengl)
                else()
                    message(WARNING "OpenGL or GLEW library not found; 'out_win' app won't build")
                endif()

                pkg_check_modules(CAIRO QUIET cairo)
                if(CAIRO_FOUND)
                    add_library(out_win_cairo SHARED "${XAVA_MODULE_DIR}/main.c"
                        "src/output/shared/graphical.c"
                        "src/output/shared/cairo/main.c"
                        "src/output/shared/cairo/util/module.c"
                        "src/output/shared/cairo/util/feature_compat.c"
                        "src/output/shared/cairo/util/region.c"
                        "${GLOBAL_FUNCTION_SOURCES}")
                    target_link_libraries(out_win_cairo xava-shared "-lgdi32"
                        "-lwinmm -ldwmapi" ${CAIRO_LIBRARIES})
                    target_link_directories(out_win_cairo PRIVATE
                        ${CAIRO_LIBRARY_DIRS})
                    target_include_directories(out_win_cairo PRIVATE
                        "${CAIRO_INCLUDE_DIRS}")
                    target_compile_definitions(out_win_cairo PUBLIC -DWIN -DCAIRO)
                    set_target_properties(out_win_cairo PROPERTIES PREFIX "")

                    find_and_copy_dlls(out_win_cairo)
                else()
                    message(WARNING "Cairo library not found, \"win_cairo\" won't build")
                endif()
            else()
                message(WARNING "DWMAPI library not found, WinAPI app won't build")
            endif()
        else()
            message(WARNING "GLU library not found, WinAPI app won't build")
        endif()
    else()
        message(WARNING "GDI library not found, WinAPI app won't build")
    endif()
endif()
