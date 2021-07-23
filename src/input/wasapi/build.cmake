# The project default
option(WASAPI "WASAPI" ON)

if(NOT (MSYS OR MINGW OR MSVC))
	set(WASAPI OFF)
endif()

# winapi
if(WASAPI)
	add_library(in_wasapi SHARED "${XAVA_MODULE_DIR}/main.cpp"
		"${GLOBAL_FUNCTION_SOURCES}")
	target_link_libraries(in_wasapi xava-shared ole32 oleaut32)
	#target_include_directories(in_wasapi PRIVATE "${ALSA_INCLUDE_DIRS}")
	#target_link_directories(in_wasapi PRIVATE "${ALSA_LIBRARY_DIRS}")
	set_target_properties(in_wasapi PROPERTIES PREFIX "")
endif()

