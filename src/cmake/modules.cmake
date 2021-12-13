# Build GL modules
file(GLOB gl_modules "src/output/shared/gl/modules/*/build.cmake")
foreach(dir ${gl_modules})
    get_filename_component(XAVA_MODULE_DIR ${dir} DIRECTORY)
    include(${dir})
endforeach()

# Build Cairo modules
file(GLOB cairo_modules "src/output/shared/cairo/modules/*/build.cmake")
foreach(dir ${cairo_modules})
    get_filename_component(XAVA_MODULE_DIR ${dir} DIRECTORY)
    include(${dir})
endforeach()

# Build filter modules
file(GLOB filters "src/filters/*/build.cmake" )
foreach(dir ${filters})
    get_filename_component(XAVA_MODULE_DIR ${dir} DIRECTORY)
    include(${dir})
endforeach()

# Build output modules
file(GLOB outputs "src/output/*/build.cmake" )
foreach(dir ${outputs})
    # Exclude the default module
    if("${dir}" STREQUAL "${CMAKE_SOURCE_DIR}/src/output/default/build.cmake")
        continue()
    endif()

    get_filename_component(XAVA_MODULE_DIR ${dir} DIRECTORY)
    include(${dir})
endforeach()
# This is so that the default module gets build AFTER everything else
get_filename_component(XAVA_MODULE_DIR "src/output/default/build.cmake" DIRECTORY)
include("src/output/default/build.cmake")

# Build unsupported output modules
if(BUILD_LEGACY_OUTPUTS)
    file(GLOB legacy_outputs "src/output/unsupported/*/build.cmake" )
    foreach(dir ${legacy_outputs})
        get_filename_component(XAVA_MODULE_DIR ${dir} DIRECTORY)
        include(${dir})
    endforeach()
endif()

# Build input modules
file(GLOB inputs "src/input/*/build.cmake" )
foreach(dir ${inputs})
    get_filename_component(XAVA_MODULE_DIR ${dir} DIRECTORY)
    include(${dir})
endforeach()
