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

                set(GLEW_USE_STATIC_LIBS ON)
                find_package(GLEW)
                if(WGL_LIB AND GLEW)
                    add_library(out_win SHARED "${XAVA_MODULE_DIR}/main.c"
                        "src/output/shared/graphical.c"
                        "src/output/shared/gl/glew.c"
                        "src/output/shared/gl/main.c"
                        "${GLOBAL_FUNCTION_SOURCES}")
                    add_definitions(-DGLEW_STATIC)
                    target_link_libraries(out_win xava-shared GLEW::glew_s
                        "-lgdi32 -lwinmm -lopengl32 -ldwmapi")
                    target_compile_definitions(out_win PUBLIC -DWIN -DGL
                        -DGLEW_STATIC)
                    set_target_properties(out_win PROPERTIES PREFIX "")

                    # copy dependency dll's because fucking windows
                    add_custom_command(TARGET out_win POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E env MINGW_BUNDLEDLLS_SEARCH_PATH="./:${xava_dep_dirs}"
                        python "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/mingw-bundledlls/mingw-bundledlls" $<TARGET_FILE:out_win> --copy
                    )
                else()
                    message(WARNING "OpenGL library not found, WinAPI app won't build")
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

                    add_custom_command(TARGET out_win_cairo POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E env MINGW_BUNDLEDLLS_SEARCH_PATH="./:${xava_dep_dirs}"
                        python "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/mingw-bundledlls/mingw-bundledlls" $<TARGET_FILE:out_win_cairo> --copy
                    )
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
