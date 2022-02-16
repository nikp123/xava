# Project default
option(GL_MODULES "GL_MODULES" ON)

# Xorg
if(GL_MODULES)
    # OpenGL/GLEW
    find_library(GLEW glew32)
    pkg_check_modules(GLEW QUIET glew)
    if(GLEW OR GLEW_FOUND)
        add_library(gl_bars_circle SHARED "${XAVA_MODULE_DIR}/main.c"
                                    "${XAVA_MODULE_DIR}/../../util/shader.c"
                                    "${XAVA_MODULE_DIR}/../../util/misc.c"
                                    "${GLOBAL_FUNCTION_SOURCES}")
        target_link_directories(gl_bars_circle PRIVATE 
            "${GLEW_LIBRARY_DIRS}")
        target_include_directories(gl_bars_circle PRIVATE 
            "${GLEW_INCLUDE_DIRS}")

        if(WINDOWS OR MINGW OR MSVC OR CYGWIN)
            target_link_libraries(gl_bars_circle xava-shared "-lglew32 -lopengl32")
        else()
            target_link_libraries(gl_bars_circle xava-shared "${GLEW_LIBRARIES}")
        endif()
        target_compile_definitions(gl_bars_circle PUBLIC -DGL)

        set_target_properties(gl_bars_circle PROPERTIES PREFIX "")
        set_target_properties(gl_bars_circle PROPERTIES IMPORT_PREFIX "")
        set_target_properties(gl_bars_circle PROPERTIES OUTPUT_NAME "gl/module/bars_circle/module")

        # this copies the dlls for mr. windows
        #find_and_copy_dlls(gl_bars_circle)

        configure_file("${XAVA_MODULE_DIR}/vertex.glsl"   gl/module/bars_circle/vertex.glsl   COPYONLY)
        configure_file("${XAVA_MODULE_DIR}/fragment.glsl" gl/module/bars_circle/fragment.glsl COPYONLY)
        configure_file("${XAVA_MODULE_DIR}/config.ini"    gl/module/bars_circle/config.ini    COPYONLY)
        install(TARGETS gl_bars_circle RENAME module DESTINATION share/xava/gl/module/bars_circle/)
        install(FILES "${CMAKE_BINARY_DIR}/gl/module/bars_circle/vertex.glsl"   RENAME vertex.glsl.example   DESTINATION share/xava/gl/module/bars_circle/)
        install(FILES "${CMAKE_BINARY_DIR}/gl/module/bars_circle/fragment.glsl" RENAME fragment.glsl.example DESTINATION share/xava/gl/module/bars_circle/)
        install(FILES "${CMAKE_BINARY_DIR}/gl/module/bars_circle/config.ini"    RENAME config.ini.example    DESTINATION share/xava/gl/module/bars_circle/)

        # Maybe GL license?
    else()
        message(WARNING "GLEW library not found; \"gl_bars_circle\" won't build")
    endif()
endif()
