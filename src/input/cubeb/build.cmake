# Project default
option(CUBEB "CUBEB" OFF)

# libcubeb-0.3
if(CUBEB)
    set(BUILD_SHARED_LIBS  ON)
    set(BUILD_TESTS       OFF)
    set(BUILD_RUST_LIBS    ON)
    set(BUILD_TOOLS       OFF)
    set(BUNDLE_SPEEX      OFF)
    set(LAZY_LOAD_LIBS    OFF)
    set(USE_SANITIZERS    OFF)

    execute_process(COMMAND git submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/cubeb
        COMMAND_ERROR_IS_FATAL ANY)

    add_subdirectory(lib/cubeb)
    set_target_properties(cubeb PROPERTIES POSITION_INDEPENDENT_CODE ON)

    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/lib/cubeb/cmake_install.cmake")

    add_definitions(-DCUBEB)
    add_library(in_cubeb SHARED "${XAVA_MODULE_DIR}/main.c"
                                "${GLOBAL_FUNCTION_SOURCES}")
    target_link_libraries(in_cubeb xava-shared "cubeb")
    target_include_directories(in_cubeb PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/exports" lib/cubeb/include)
    set_target_properties(in_cubeb PROPERTIES PREFIX ""
        POSITION_INDEPENDENT_CODE ON)
    install(TARGETS in_cubeb DESTINATION lib/xava)

    # Add legal disclaimer
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_cubeb.txt"
"cubeb license can be obtained at: \
https://raw.githubusercontent.com/mozilla/cubeb/master/LICENSE\n")
endif()

