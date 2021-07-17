# Project default
option(X11 "X11" ON)

# Xorg
if(X11)
	if(APPLE)
		include(FindX11)
		if(X11_FOUND)
			add_definitions(-DXLIB)
			add_library(out_x11 SHARED "${XAVA_MODULE_DIR}/main.c"
										"src/output/graphical.c"
										"${GLOBAL_FUNCTION_SOURCES}")
			target_link_directories(out_x11 PRIVATE "${X11_INCLUDE_DIR}"
				"${X11_Xfixes_INCLUDE_PATH}" "${X11_Xrandr_INCLUDE_PATH}" )
			target_include_directories(out_x11 PRIVATE "${X11_INCLUDE_DIR}"
				"${X11_Xfixes_INCLUDE_PATH}" "${X11_Xrandr_INCLUDE_PATH}" )
			target_link_libraries(out_x11 xava-shared "${X11_LIBRARIES}"
				"${X11_Xfixes_LIB}" "${X11_Xrandr_LIB}" iniparser)
			set_target_properties(out_x11 PROPERTIES PREFIX "")
			install(TARGETS out_x11 DESTINATION lib/xava)
		else()
			message(STATUS "X11, Xrandr and/or Xfixes library not found")
		endif()
	else()
		pkg_check_modules(X11 QUIET x11 xfixes xrandr)
		if(X11_FOUND)
			add_definitions(-DXLIB)
			add_library(out_x11 SHARED "${XAVA_MODULE_DIR}/main.c"
											"src/output/graphical.c"
											"${GLOBAL_FUNCTION_SOURCES}")
			target_link_directories(out_x11 PRIVATE "${X11_LIBRARY_DIRS}")
			target_include_directories(out_x11 PRIVATE "${X11_INCLUDE_DIRS}")
			target_link_libraries(out_x11 xava-shared "${X11_LIBRARIES}" iniparser)
			set_target_properties(out_x11 PROPERTIES PREFIX "")
			install(TARGETS out_x11 DESTINATION lib/xava)

			# Stars
			add_library(out_x11_stars SHARED "${XAVA_MODULE_DIR}/main.c"
											"src/output/graphical.c"
											"${GLOBAL_FUNCTION_SOURCES}")
			target_link_directories(out_x11_stars PRIVATE "${X11_LIBRARY_DIRS}")
			target_include_directories(out_x11_stars PRIVATE "${X11_INCLUDE_DIRS}")
			target_link_libraries(out_x11_stars xava-shared "${X11_LIBRARIES}" iniparser)
			set_target_properties(out_x11_stars PROPERTIES PREFIX "")
			install(TARGETS out_x11_stars DESTINATION lib/xava)
			target_compile_definitions(out_x11_stars PUBLIC -DSTARS)

			# EGL
			pkg_check_modules(EGL QUIET egl glesv2)
			if(EGL_FOUND)
				add_library(out_x11_egl SHARED "${XAVA_MODULE_DIR}/main.c"
											"src/output/graphical.c"
											"src/output/shared/egl.c"
											"src/output/shared/gl_shared.c"
											"${GLOBAL_FUNCTION_SOURCES}")
				target_link_directories(out_x11_egl PRIVATE 
					"${EGL_LIBRARY_DIRS}" "${X11_LIBRARY_DIRS}")
				target_include_directories(out_x11_egl PRIVATE 
					"${EGL_INCLUDE_DIRS}" "${X11_INCLUDE_DIRS}")
				target_link_libraries(out_x11_egl xava-shared 
					"${EGL_LIBRARIES}" "${X11_LIBRARIES}" iniparser)
				target_compile_definitions(out_x11_egl PUBLIC -DEGL)
				set_target_properties(out_x11_egl PROPERTIES PREFIX "")
				install(TARGETS out_x11_egl DESTINATION lib/xava)
			else()
				message(STATUS "EGL and or Xrender library not found")
			endif()
		else()
			message(STATUS "X11, Xrandr and/or Xfixes library not found")
		endif()
	endif()
endif()
