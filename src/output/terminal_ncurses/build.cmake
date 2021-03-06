# Project default
option(NCURSES "NCURSES" ON)

# NCURSES
if(NCURSES)
	pkg_check_modules(NCURSES QUIET ncurses)
	if(NCURSES_FOUND)
		add_library(out_ncurses SHARED 
			"${XAVA_MODULE_DIR}/main.c"
			"${GLOBAL_FUNCTION_SOURCES}")
		target_link_libraries(out_ncurses "${NCURSES_LIBRARIES}" iniparser)
		target_include_directories(out_ncurses PRIVATE "${NCURSES_INCLUDE_DIRS}")
		target_link_directories(out_ncurses PRIVATE "${NCURSES_LIBRARY_DIRS}")
		set_target_properties(out_ncurses PROPERTIES PREFIX "")
		install(TARGETS out_ncurses DESTINATION lib/xava)
	else()
		message(STATUS "ncurses library not found")
	endif()
endif()
