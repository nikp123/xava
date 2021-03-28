# The project default
option(PULSEAUDIO "PULSEAUDIO" ON)

# pulseaudio
if(PULSEAUDIO)
	pkg_check_modules(PULSEAUDIO QUIET libpulse libpulse-simple)
	if(PULSEAUDIO_FOUND)
		add_definitions(-DPULSE)
		add_library(in_pulseaudio SHARED "${XAVA_MODULE_DIR}/main.c"
										"${GLOBAL_FUNCTION_SOURCES}")
		target_link_libraries(in_pulseaudio "${PULSEAUDIO_LIBRARIES}" iniparser)
		target_include_directories(in_pulseaudio PRIVATE "${PULSEAUDIO_INCLUDE_DIRS}")
		target_link_directories(in_pulseaudio PRIVATE "${PULSEAUDIO_LIBRARY_DIRS}")
		set_target_properties(in_pulseaudio PROPERTIES PREFIX "")
		install(TARGETS in_pulseaudio DESTINATION lib/xava)
	else()
		message(STATUS "pulseaudio library not found")
	endif()
endif()

