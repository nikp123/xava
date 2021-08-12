# The project default
option(PORTAUDIO "PORTAUDIO" ON)

if(PORTAUDIO)
	pkg_check_modules(PORTAUDIO QUIET portaudio-2.0)
	if(PORTAUDIO_FOUND)
		add_definitions(-DPORTAUDIO)
		add_library(in_portaudio SHARED "${XAVA_MODULE_DIR}/main.c"
										"${GLOBAL_FUNCTION_SOURCES}")
		target_link_libraries(in_portaudio xava-shared "${PORTAUDIO_LIBRARIES}")
		target_include_directories(in_portaudio PRIVATE "${PORTAUDIO_INCLUDE_DIRS}")
		target_link_directories(in_portaudio PRIVATE "${PORTAUDIO_LIBRARY_DIRS}")
		set_target_properties(in_portaudio PROPERTIES PREFIX "")
		install(TARGETS in_portaudio DESTINATION lib/xava)

		# Add legal disclaimer
		file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_portaudio.txt"
			"PortAudio license can be obtained at: http://www.portaudio.com/license.html\n")
	else()
		message(WARNING "PortAudio library not found, PortAudio won't build")
	endif()
endif()
