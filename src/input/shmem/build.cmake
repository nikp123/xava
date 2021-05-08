# Project default
option(SHMEM "SHMEM" OFF)

if(SHMEM)
	add_definitions(-DSHMEM)
	add_library(in_shmem SHARED "${XAVA_MODULE_DIR}/main.c"
								"${GLOBAL_FUNCTION_SOURCES}")
	target_link_libraries(in_shmem xava-log "-lrt" iniparser)
	set_target_properties(in_shmem PROPERTIES PREFIX "")
	install(TARGETS in_shmem DESTINATION lib/xava)
endif()

