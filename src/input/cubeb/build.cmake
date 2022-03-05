# Project default
option(CUBEB "CUBEB" ON)

# libcubeb-0.3
if(CUBEB)
    set(BUILD_SHARED_LIBS ON)
    set(BUILD_TESTS       ON)
    set(BUILD_RUST_LIBS   ON)
    set(BUILD_TOOLS      OFF)
    set(BUNDLE_SPEEX     OFF)
    set(LAZY_LOAD_LIBS    ON)
    set(USE_SANITIZERS   OFF)
    add_subdirectory(lib/cubeb)

    add_definitions(-DCUBEB)
    add_library(in_cubeb SHARED "${XAVA_MODULE_DIR}/main.c"
                                "${GLOBAL_FUNCTION_SOURCES}")
    target_link_libraries(in_cubeb xava-shared "cubeb")
    target_include_directories(in_cubeb PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/exports" lib/cubeb/include)
    set_target_properties(in_cubeb PROPERTIES PREFIX "")
    install(TARGETS in_cubeb DESTINATION lib/xava)

    # Add legal disclaimer
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_cubeb.txt"
"cubeb license can be obtained at: \
https://raw.githubusercontent.com/mozilla/cubeb/master/LICENSE\n")
endif()

