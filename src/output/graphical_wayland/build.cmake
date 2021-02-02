# Project default
option(WAYLAND "WAYLAND" ON)

# SDL2
if(SDL2)
	pkg_check_modules(SDL2 QUIET sdl2)
	if(SDL2_FOUND)
		add_definitions(-DSDL)
		add_library(out_sdl2 SHARED "${XAVA_MODULE_DIR}/main.c")
		target_link_libraries(out_sdl2 "${SDL2_LIBRARIES}")
		target_include_directories(out_sdl2 PRIVATE "${SDL2_INCLUDE_DIRS}")
		target_link_directories(out_sdl2 PRIVATE "${SDL2_LIBRARY_DIRS}")
		set_target_properties(out_sdl2 PROPERTIES PREFIX "")
		install(TARGETS out_sdl2 DESTINATION lib/xava)
	else()
		message(STATUS "SDL2 library not found")
	endif()
endif()

# Wayland
if(WAYLAND)
	# SHMEM is a MUST dependency
	set(SHMEM ON)

	pkg_check_modules(WAYLAND QUIET egl wayland-client wayland-egl)
	if(WAYLAND_FOUND)
		# Hacky way to deal with wayland-scanner (but will be used for nw)
		execute_process(COMMAND wayland-scanner client-header 
			"${WL_PROT_DIR}/stable/xdg-shell/xdg-shell.xml"
			"${XAVA_MODULE_DIR}/xdg-shell-client-protocol.h")
		execute_process(COMMAND wayland-scanner client-header
			"${CMAKE_CURRENT_LIST_DIR}/assets/linux/wayland/protocols/wlr-layer-shell-unstable-v1.xml"
			"${XAVA_MODULE_DIR}/wlr-layer-shell-unstable-v1-client-protocol.h")
		execute_process(COMMAND wayland-scanner private-code
			"${CMAKE_CURRENT_LIST_DIR}/assets/linux/wayland/protocols/wlr-layer-shell-unstable-v1.xml"
			"${XAVA_MODULE_DIR}/wlr-layer-shell-unstable-v1-client-protocol.c")
		execute_process(COMMAND wayland-scanner client-header
			"${CMAKE_CURRENT_LIST_DIR}/assets/linux/wayland/protocols/wlr-output-management-unstable-v1.xml"
			"${XAVA_MODULE_DIR}/wlr-output-managment-unstable-v1.h")
		execute_process(COMMAND wayland-scanner private-code
			"${CMAKE_CURRENT_LIST_DIR}/assets/linux/wayland/protocols/wlr-output-management-unstable-v1.xml"
			"${XAVA_MODULE_DIR}/wlr-output-managment-unstable-v1.c")
		execute_process(COMMAND wayland-scanner client-header
			"${WL_PROT_DIR}/unstable/xdg-output/xdg-output-unstable-v1.xml"
			"${XAVA_MODULE_DIR}/xdg-output-unstable-v1-client-protocol.h")

		add_library(out_wayland SHARED
			"${XAVA_MODULE_DIR}/main.c"
			"${XAVA_MODULE_DIR}/wlr-output-managment-unstable-v1.c"
			"${XAVA_MODULE_DIR}/wlr-layer-shell-unstable-v1-client-protocol.c")
		target_link_libraries(out_wayland "${WAYLAND_LIBRARIES}")
		target_include_directories(out_wayland PRIVATE "${WAYLAND_INCLUDE_DIRS}")
		target_link_directories(out_wayland PRIVATE "${WAYLAND_LIBRARY_DIRS}")
		set_target_properties(out_wayland PROPERTIES PREFIX "")
		install(TARGETS out_wayland DESTINATION lib/xava)
		add_definitions(-DWAYLAND)
	else()
		message(STATUS "Wayland libraries not found")
	endif()
endif()
