# The project default
option(WINAPI "WINAPI" ON)

if(NOT (MSYS OR MINGW OR MSVC))
	set(WINAPI OFF)
endif()

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
						"${GLOBAL_FUNCTION_SOURCES}")
					target_link_libraries(out_win "-lgdi32 -lwinmm -lopengl32 -lglu32 -ldwmapi" iniparser)
					target_compile_definitions(out_win PUBLIC -DWIN -DGL)
					set_target_properties(out_win PROPERTIES PREFIX "")
				else()
					message("DWMAPI library not found")
				endif()
			else()
				message("GLU library not found")
			endif()
		else()
			message("openGL library not found")
		endif()
	else()
		message(STATUS "GDI library not found")
	endif()

	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		message(STATUS "Since release build, console is being disabled")
		SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS} -mwindows")

		# Prepare NSI file for compilation
		configure_file("assets/windows/xava.nsi.template" "xava.nsi" NEWLINE_STYLE CRLF)
	endif()
endif()
