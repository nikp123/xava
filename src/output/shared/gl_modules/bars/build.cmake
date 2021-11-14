# Project default
option(GL_MODULES "GL_MODULES" ON)

# Xorg
if(GL_MODULES)
    # OpenGL/GLEW
    pkg_check_modules(GLEW QUIET glew)
    if(GLEW_FOUND)
        add_library(gl_bars SHARED "${XAVA_MODULE_DIR}/main.c"
                                    "${XAVA_MODULE_DIR}/../shared/shader.c"
                                    "${XAVA_MODULE_DIR}/../shared/post.c"
                                    "${XAVA_MODULE_DIR}/../shared/util.c"
                                    "${GLOBAL_FUNCTION_SOURCES}")
        target_link_directories(gl_bars PRIVATE 
            "${GLEW_LIBRARY_DIRS}")
        target_include_directories(gl_bars PRIVATE 
            "${GLEW_INCLUDE_DIRS}")
        if(WINDOWS OR MINGW OR MSVC OR CYGWIN)
            target_link_libraries(gl_bars xava-shared GLEW::glew_s "-lopengl32")
            target_compile_definitions(gl_bars PUBLIC -DGL -DGLEW_STATIC)
        else()
            target_link_libraries(gl_bars xava-shared 
                "${GLEW_LIBRARIES}")
            target_compile_definitions(gl_bars PUBLIC -DGL)
        endif()
        set_target_properties(gl_bars PROPERTIES PREFIX "")
        set_target_properties(gl_bars PROPERTIES IMPORT_PREFIX "")
        set_target_properties(gl_bars PROPERTIES OUTPUT_NAME "gl/module/bars/module")
        configure_file("${XAVA_MODULE_DIR}/vertex.glsl"   gl/module/bars/vertex.glsl   COPYONLY)
        configure_file("${XAVA_MODULE_DIR}/fragment.glsl" gl/module/bars/fragment.glsl COPYONLY)
        install(TARGETS gl_bars RENAME module DESTINATION share/xava/gl/module/bars/)
        install(FILES "${CMAKE_BINARY_DIR}/gl/module/bars/vertex.glsl"   RENAME vertex.glsl.example   DESTINATION share/xava/gl/module/bars/)
        install(FILES "${CMAKE_BINARY_DIR}/gl/module/bars/fragment.glsl" RENAME fragment.glsl.example DESTINATION share/xava/gl/module/bars/)

        # Maybe GL license?
    else()
        message(WARNING "GLEW library not found; \"gl_bars\" won't build")
    endif()
endif()
