# Project default
option(SDL2 "SDL2" ON)

# SDL2
if(SDL2)
	pkg_check_modules(SDL2 QUIET sdl2)
	if(SDL2_FOUND)
		add_library(out_sdl2_osciloscope SHARED
			"${XAVA_MODULE_DIR}/main.c"
			"src/output/graphical.c"
			"${GLOBAL_FUNCTION_SOURCES}")
		target_link_libraries(out_sdl2_osciloscope xava-shared "${SDL2_LIBRARIES}")
		target_include_directories(out_sdl2_osciloscope PRIVATE "${SDL2_INCLUDE_DIRS}")
		target_link_directories(out_sdl2_osciloscope PRIVATE "${SDL2_LIBRARY_DIRS}")
		set_target_properties(out_sdl2_osciloscope PROPERTIES PREFIX "")
		target_compile_definitions(out_sdl2_osciloscope PUBLIC -DSDL)
		install(TARGETS out_sdl2_osciloscope DESTINATION lib/xava)

		# Add legal disclaimer
		file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_sdl2.txt"
			"SDL2 license can be obtained at: https://www.libsdl.org/license.php\n")
	else()
		message(WARNING "SDL2 library not found")
	endif()
endif()
