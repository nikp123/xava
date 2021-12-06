# Default filter, is well..... default
option(FILTER_DEFAULT "FILTER_DEFAULT" ON)

# fftw3
pkg_check_modules(FFTW3 REQUIRED fftw3f)
list(APPEND INCLUDE_DIRS "${FFTW3_INCLUDE_DIRS}")
list(APPEND LINK_DIRS "${FFTW3_LIBRARY_DIRS}")

if(FILTER_DEFAULT)
	message(STATUS "Default filter enabled!")
	add_library(filter_default SHARED "${XAVA_MODULE_DIR}/main.c"
										"${GLOBAL_FUNCTION_SOURCES}")
	target_link_libraries(filter_default xava-shared "${FFTW3_LIBRARIES}")
	set_target_properties(filter_default PROPERTIES PREFIX "")
	install(TARGETS filter_default DESTINATION lib/xava)

	# 1001 reasons to not write shit in C
	if(MINGW)
		add_custom_command(TARGET filter_default POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E env MINGW_BUNDLEDLLS_SEARCH_PATH="${xava_dep_dirs}"
			python "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/mingw-bundledlls/mingw-bundledlls" $<TARGET_FILE:filter_default> --copy
		)
	endif()

	# Add legal disclaimer
	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_fftw.txt"
		"FFTW License can be obtained at: http://fftw.org/doc/License-and-Copyright.html\n")
endif()

