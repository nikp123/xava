# The project default
option(SNDIO "SNDIO" ON)

# sndio
if(SNDIO)
	find_library(SNDIO_LIB sndio HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
	if(SNDIO_LIB)
		add_definitions(-DSNDIO)
		add_library(in_sndio SHARED "${XAVA_MODULE_DIR}/main.c"
									"${GLOBAL_FUNCTION_SOURCES}")
		target_link_libraries(in_sndio xava-shared "-lsndio")
		set_target_properties(in_sndio PROPERTIES PREFIX "")
		install(TARGETS in_sndio DESTINATION lib/xava)
	else()
		message(STATUS "sndio library not found")
	endif()
endif()

