# Project default
option(SDL2 "SDL2" ON)

# SDL2
if(SDL2)
	pkg_check_modules(SDL2 QUIET sdl2)
	if(SDL2_FOUND)
		add_definitions(-DSDL)
		add_library(out_sdl2 SHARED 
			"${XAVA_MODULE_DIR}/main.c"
			"src/output/graphical.c"
			"${GLOBAL_FUNCTION_SOURCES}")
		target_link_libraries(out_sdl2 xava-log "${SDL2_LIBRARIES}" iniparser)
		target_include_directories(out_sdl2 PRIVATE "${SDL2_INCLUDE_DIRS}")
		target_link_directories(out_sdl2 PRIVATE "${SDL2_LIBRARY_DIRS}")
		set_target_properties(out_sdl2 PROPERTIES PREFIX "")
		install(TARGETS out_sdl2 DESTINATION lib/xava)
	else()
		message(STATUS "SDL2 library not found")
	endif()
endif()
