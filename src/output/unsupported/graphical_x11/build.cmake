# Project default
option(X11 "X11" ON)

# Xorg
if(X11)
	if(APPLE)
		include(FindX11)
		if(X11_FOUND)
			add_definitions(-DXLIB)
			add_library(out_x11_sw SHARED "${XAVA_MODULE_DIR}/main.c"
										"src/output/graphical.c"
										"${GLOBAL_FUNCTION_SOURCES}")
			target_link_directories(out_x11_sw PRIVATE "${X11_INCLUDE_DIR}"
				"${X11_Xfixes_INCLUDE_PATH}" "${X11_Xrandr_INCLUDE_PATH}" )
			target_include_directories(out_x11_sw PRIVATE "${X11_INCLUDE_DIR}"
				"${X11_Xfixes_INCLUDE_PATH}" "${X11_Xrandr_INCLUDE_PATH}" )
			target_link_libraries(out_x11_sw xava-shared "${X11_LIBRARIES}"
				"${X11_Xfixes_LIB}" "${X11_Xrandr_LIB}")
			set_target_properties(out_x11_sw PROPERTIES PREFIX "")
			install(TARGETS out_x11_sw DESTINATION lib/xava)
		else()
			message(WARNING "X11, Xrandr and/or Xfixes library not found; X11 won't build")
		endif()
	else()
		pkg_check_modules(X11 QUIET x11 xfixes xrandr)
		if(X11_FOUND)
			add_definitions(-DXLIB)
			add_library(out_x11_sw SHARED "${XAVA_MODULE_DIR}/main.c"
											"src/output/graphical.c"
											"${GLOBAL_FUNCTION_SOURCES}")
			target_link_directories(out_x11_sw PRIVATE "${X11_LIBRARY_DIRS}")
			target_include_directories(out_x11_sw PRIVATE "${X11_INCLUDE_DIRS}")
			target_link_libraries(out_x11_sw xava-shared "${X11_LIBRARIES}")
			set_target_properties(out_x11_sw PROPERTIES PREFIX "")
			install(TARGETS out_x11_sw DESTINATION lib/xava)

			# Stars
			add_library(out_x11_sw_stars SHARED "${XAVA_MODULE_DIR}/main.c"
											"src/output/graphical.c"
											"${GLOBAL_FUNCTION_SOURCES}")
			target_link_directories(out_x11_sw_stars PRIVATE "${X11_LIBRARY_DIRS}")
			target_include_directories(out_x11_sw_stars PRIVATE "${X11_INCLUDE_DIRS}")
			target_link_libraries(out_x11_sw_stars xava-shared "${X11_LIBRARIES}")
			set_target_properties(out_x11_sw_stars PROPERTIES PREFIX "")
			install(TARGETS out_x11_sw_stars DESTINATION lib/xava)
			target_compile_definitions(out_x11_sw_stars PUBLIC -DSTARS)

			# Add legal disclaimer
			file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_x11.txt" 
				"X11 license can be obtained at: https://raw.githubusercontent.com/mirror/libX11/master/COPYING\n")
		else()
			message(WARNING "X11, Xrandr and/or Xfixes library not found; X11 won't build")
		endif()
	endif()
endif()
