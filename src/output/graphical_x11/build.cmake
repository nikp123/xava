# Project default
option(X11 "X11" ON)

# Xorg
if(X11)
	if(APPLE)
		message(WARNING "Mate, you Apples need to use the legacy thing now.")
	else()
		pkg_check_modules(X11 QUIET x11 xfixes xrandr)
		if(X11_FOUND)
			# OpenGL/GLX
			pkg_check_modules(GLX QUIET gl xrender glew)
			if(GLX_FOUND)
				add_library(out_x11 SHARED "${XAVA_MODULE_DIR}/main.c"
											"src/output/graphical.c"
											"src/output/shared/glew.c"
											"src/output/shared/gl_shared.c"
											"${GLOBAL_FUNCTION_SOURCES}")
				target_link_directories(out_x11 PRIVATE 
					"${GLX_LIBRARY_DIRS}" "${X11_LIBRARY_DIRS}")
				target_include_directories(out_x11 PRIVATE 
					"${GLX_INCLUDE_DIRS}" "${X11_INCLUDE_DIRS}")
				target_link_libraries(out_x11 xava-shared 
					"${GLX_LIBRARIES}" "${X11_LIBRARIES}")
				target_compile_definitions(out_x11 PUBLIC -DGL)
				set_target_properties(out_x11 PROPERTIES PREFIX "")
				install(TARGETS out_x11 DESTINATION lib/xava)

				# Maybe GL license?
			else()
				message(WARNING "GLEW, OpenGL and or Xrender library not found; \"x11_gl\" won't build")
			endif()

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
				target_link_libraries(out_x11_egl xava-shared GLEW 
					"${EGL_LIBRARIES}" "${X11_LIBRARIES}")
				target_compile_definitions(out_x11_egl PUBLIC -DEGL)
				set_target_properties(out_x11_egl PROPERTIES PREFIX "")
				install(TARGETS out_x11_egl DESTINATION lib/xava)

				# Maybe EGL license?
			else()
				message(WARNING "EGL and or GLESv2 library not found; \"x11_egl\" won't build")
			endif()

			# Add legal disclaimer
			file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_x11.txt" 
				"X11 license can be obtained at: https://raw.githubusercontent.com/mirror/libX11/master/COPYING\n")
		else()
			message(WARNING "X11, Xrandr and/or Xfixes library not found; X11 won't build")
		endif()
	endif()
endif()
