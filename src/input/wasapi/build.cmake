# The project default
option(WASAPI "WASAPI" ON)

if(NOT (MSYS OR MINGW OR MSVC))
	set(WASAPI OFF)
endif()

# winapi
if(WASAPI)
	list(APPEND ADDITIONAL_SOURCES "${XAVA_MODULE_DIR}/main.cpp")
endif()
