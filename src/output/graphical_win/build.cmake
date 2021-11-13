# The project default
option(WINAPI "WINAPI" ON)

if(NOT (MSYS OR MINGW OR MSVC))
	set(WINAPI OFF)
endif()

set(GLEW_USE_STATIC_LIBS ON)
find_package(GLEW)

# winapi
if(WINAPI)
	find_library(GDI_LIB gdi32 HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
	if(GDI_LIB)
		find_library(WGL_LIB opengl32 HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
		if(WGL_LIB)
			find_library(GLU_LIB glu32 HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
			if(GLU_LIB)
				find_library(DWM_LIB dwmapi HINTS ${CMAKE_C_IMPLICIT_LINK_DIRECTORIES})
				if(DWM_LIB)
					add_library(out_win SHARED "${XAVA_MODULE_DIR}/main.c"
						"src/output/graphical.c"
						"src/output/shared/glew.c"
						"src/output/shared/gl_shared.c"
						"${GLOBAL_FUNCTION_SOURCES}")
					add_definitions(-DGLEW_STATIC)
					target_link_libraries(out_win xava-shared GLEW::glew_s "-lgdi32 -lwinmm -lopengl32 -ldwmapi")
					target_compile_definitions(out_win PUBLIC -DWIN -DGL -DGLEW_STATIC)
					set_target_properties(out_win PROPERTIES PREFIX "")
				else()
					message(WARNING "DWMAPI library not found, WinAPI app won't build")
				endif()
			else()
				message(WARNING "GLU library not found, WinAPI app won't build")
			endif()
		else()
			message(WARNING "OpenGL library not found, WinAPI app won't build")
		endif()
	else()
		message(WARNING "GDI library not found, WinAPI app won't build")
	endif()
endif()
