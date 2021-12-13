
foreach(name out_cairo out_opengl)
    add_library(${name} SHARED
        "${XAVA_MODULE_DIR}/main.c"
        ${DEFAULT_OUTPUT_SOURCES}
        "${GLOBAL_FUNCTION_SOURCES}")
    
    target_link_libraries     (${name} xava-shared ${DEFAULT_OUTPUT_LINKLIB})
    target_include_directories(${name} PRIVATE     ${DEFAULT_OUTPUT_INCDIR})
    target_link_directories   (${name} PRIVATE     ${DEFAULT_OUTPUT_LINKDIR})
    set_target_properties     (${name} PROPERTIES PREFIX "")

    if(${name} STREQUAL "out_cairo")
        set(OUTPUT_DEFAULT_DEFINE "-DCAIRO")
    elseif(${name} STREQUAL "out_opengl")
        set(OUTPUT_DEFAULT_DEFINE "-DOPENGL")
    endif()

    target_compile_definitions(${name} PUBLIC ${OUTPUT_DEFAULT_DEFINE} ${DEFAULT_OUTPUT_DEF})
    
    install(TARGETS ${name} DESTINATION lib/xava)
    
    find_and_copy_dlls(${name})
endforeach()
