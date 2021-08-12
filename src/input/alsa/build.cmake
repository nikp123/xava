# Project default
option(ALSA "ALSA" ON)

# alsa
if(ALSA)
	pkg_check_modules(ALSA QUIET alsa)
	if(ALSA_FOUND)
		add_definitions(-DALSA)
		add_library(in_alsa SHARED "${XAVA_MODULE_DIR}/main.c"
									"${GLOBAL_FUNCTION_SOURCES}")
		target_link_libraries(in_alsa xava-shared "${ALSA_LIBRARIES}")
		target_include_directories(in_alsa PRIVATE "${ALSA_INCLUDE_DIRS}")
		target_link_directories(in_alsa PRIVATE "${ALSA_LIBRARY_DIRS}")
		set_target_properties(in_alsa PROPERTIES PREFIX "")
		install(TARGETS in_alsa DESTINATION lib/xava)

		# Add legal disclaimer
		file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_alsa.txt"
			"ALSA-lib license can be obtained at: https://raw.githubusercontent.com/alsa-project/alsa-lib/master/COPYING\n")
	else()
		message(WARNING "ALSA library not found, ALSA won't build")
	endif()
endif()

