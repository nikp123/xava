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
			target_link_libraries(out_x11 "${X11_LIBRARIES}"
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
			target_link_libraries(out_x11 "${X11_LIBRARIES}" iniparser)
			set_target_properties(out_x11 PROPERTIES PREFIX "")
			install(TARGETS out_x11 DESTINATION lib/xava)
			# GLX
			pkg_check_modules(GL QUIET gl xrender)
			if(GL_FOUND)
				add_library(out_glx SHARED "${XAVA_MODULE_DIR}/main.c"
											"src/output/graphical.c"
											"${GLOBAL_FUNCTION_SOURCES}")
				target_link_directories(out_glx PRIVATE 
					"${GL_LIBRARY_DIRS}" "${X11_LIBRARY_DIRS}")
				target_include_directories(out_glx PRIVATE 
					"${GL_INCLUDE_DIRS}" "${X11_INCLUDE_DIRS}")
				target_link_libraries(out_glx 
					"${GL_LIBRARIES}" "${X11_LIBRARIES}" iniparser)
				target_compile_definitions(out_glx PUBLIC -DGLX -DGL)
				set_target_properties(out_glx PROPERTIES PREFIX "")
				install(TARGETS out_glx DESTINATION lib/xava)
			else()
				message(STATUS "GL and or Xrender library not found")
			endif()
		else()
			message(STATUS "X11, Xrandr and/or Xfixes library not found")
		endif()
	endif()
endif()
