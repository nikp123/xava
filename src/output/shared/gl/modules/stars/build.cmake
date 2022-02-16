# Project default
option(GL_MODULES "GL_MODULES" ON)

# Xorg
if(GL_MODULES)
    # OpenGL/GLEW
    find_library(GLEW glew32)
    pkg_check_modules(GLEW QUIET glew)
    if(GLEW OR GLEW_FOUND)
        add_library(gl_stars SHARED "${XAVA_MODULE_DIR}/main.c"
                                    "${XAVA_MODULE_DIR}/../../util/shader.c"
                                    "${XAVA_MODULE_DIR}/../../util/misc.c"
                                    "${GLOBAL_FUNCTION_SOURCES}")
        target_link_directories(gl_stars PRIVATE
            "${GLEW_LIBRARY_DIRS}")
        target_include_directories(gl_stars PRIVATE
            "${GLEW_INCLUDE_DIRS}")

        if(WINDOWS OR MINGW OR MSVC OR CYGWIN)
            target_link_libraries(gl_stars xava-shared "-lglew32 -lopengl32")
        else()
            target_link_libraries(gl_stars xava-shared "${GLEW_LIBRARIES}")
        endif()
        target_compile_definitions(gl_stars PUBLIC -DGL)

        set_target_properties(gl_stars PROPERTIES PREFIX "")
        set_target_properties(gl_stars PROPERTIES IMPORT_PREFIX "")
        set_target_properties(gl_stars PROPERTIES OUTPUT_NAME "gl/module/stars/module")

        # this copies the dlls for mr. windows
        #find_and_copy_dlls(gl_stars)

        configure_file("${XAVA_MODULE_DIR}/vertex.glsl"   gl/module/stars/vertex.glsl   COPYONLY)
        configure_file("${XAVA_MODULE_DIR}/fragment.glsl" gl/module/stars/fragment.glsl COPYONLY)
        configure_file("${XAVA_MODULE_DIR}/config.ini"    gl/module/stars/config.ini    COPYONLY)

        install(TARGETS gl_stars RENAME module DESTINATION share/xava/gl/module/stars/)
        install(FILES "${CMAKE_BINARY_DIR}/gl/module/stars/vertex.glsl"   RENAME vertex.glsl.example   DESTINATION share/xava/gl/module/stars/)
        install(FILES "${CMAKE_BINARY_DIR}/gl/module/stars/fragment.glsl" RENAME fragment.glsl.example DESTINATION share/xava/gl/module/stars/)
        install(FILES "${CMAKE_BINARY_DIR}/gl/module/stars/config.ini"    RENAME config.ini.example    DESTINATION share/xava/gl/module/stars/)

        # Maybe GL license?
    else()
        message(WARNING "GLEW library not found; \"gl_stars\" won't build")
    endif()
endif()
