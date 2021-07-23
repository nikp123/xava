# Project default
option(SDL2 "SDL2" ON)

# SDL2
if(SDL2)
	pkg_check_modules(SDL2 QUIET sdl2)
	if(SDL2_FOUND)
		add_library(out_sdl2 SHARED 
			"${XAVA_MODULE_DIR}/main.c"
			"src/output/graphical.c"
			"${GLOBAL_FUNCTION_SOURCES}")
		target_link_libraries(out_sdl2 xava-shared "${SDL2_LIBRARIES}")
		target_include_directories(out_sdl2 PRIVATE "${SDL2_INCLUDE_DIRS}")
		target_link_directories(out_sdl2 PRIVATE "${SDL2_LIBRARY_DIRS}")
		set_target_properties(out_sdl2 PROPERTIES PREFIX "")
		target_compile_definitions(out_sdl2 PUBLIC -DSDL)
		install(TARGETS out_sdl2 DESTINATION lib/xava)

		# OpenGL support
		pkg_check_modules(GLEW QUIET sdl2 glew)
		if(GLEW_FOUND)
			add_library(out_sdl2_gl SHARED
				"${XAVA_MODULE_DIR}/main.c"
				"src/output/graphical.c"
				"src/output/shared/gl.c"
				"src/output/shared/gl_shared.c"
				"${GLOBAL_FUNCTION_SOURCES}")
			target_link_libraries(out_sdl2_gl xava-shared "${GLEW_LIBRARIES}")
			target_include_directories(out_sdl2_gl PRIVATE "${GLEW_INCLUDE_DIRS}")
			target_link_directories(out_sdl2_gl PRIVATE "${GLEW_LIBRARY_DIRS}")
			set_target_properties(out_sdl2_gl PROPERTIES PREFIX "")
			target_compile_definitions(out_sdl2_gl PUBLIC -DSDL -DGL)
			install(TARGETS out_sdl2_gl DESTINATION lib/xava)
		else()
			message(WARNING "GLEW library not found")
		endif()
	else()
		message(WARNING "SDL2 library not found")
	endif()
endif()
