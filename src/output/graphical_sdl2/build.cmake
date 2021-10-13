# Project default
option(SDL2 "SDL2" ON)

# SDL2
if(SDL2)
	pkg_check_modules(SDL2 QUIET sdl2 glew)
	if(SDL2_FOUND)
		# Deal with fucking windows
		if(MSVC OR MINGW OR MSYS)
			set(CMAKE_EXE_LINKER_FLAGS "-static")
			set(SDL2_LIBRARIES "${SDL2_LIBRARIES} -lopengl32 -lmingw32 \
				-lSDL2main -lSDL2 -mwindows -lm -ldinput8 -ldxguid -ldxerr8 \
				-luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 \
				-lversion -luuid -static-libgcc -lsetupapi -lSetupAPI")
		endif()

		add_library(out_sdl2 SHARED
			"${XAVA_MODULE_DIR}/main.c"
			"src/output/graphical.c"
			"src/output/shared/gl_shared.c"
			"src/output/shared/gl.c"
			"${GLOBAL_FUNCTION_SOURCES}")
		target_link_libraries(out_sdl2 xava-shared "${SDL2_LIBRARIES}")
		target_include_directories(out_sdl2 PRIVATE "${SDL2_INCLUDE_DIRS}")
		target_link_directories(out_sdl2 PRIVATE "${SDL2_LIBRARY_DIRS}")
		set_target_properties(out_sdl2 PROPERTIES PREFIX "")
		target_compile_definitions(out_sdl2 PUBLIC -DSDL -DGL)
		install(TARGETS out_sdl2 DESTINATION lib/xava)

		# Add legal disclaimer
		file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_sdl2.txt"
			"SDL2 license can be obtained at: https://www.libsdl.org/license.php\n")
	else()
		message(WARNING "SDL2 or GLEW library not found")
	endif()
endif()
