# Concatenate files in CMake
function(cat IN_FILE OUT_FILE)
    file(READ ${IN_FILE} CONTENTS)
    file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

# Windows-only, find and copy missing system libraries
function(find_and_copy_dlls file)
    # 1001 reasons to not write shit in C
    if(MINGW)
        add_custom_command(TARGET ${file} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E env MINGW_BUNDLEDLLS_SEARCH_PATH="${xava_dep_dirs}"
            python "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/mingw-bundledlls/mingw-bundledlls" $<TARGET_FILE:${file}> --copy
        )
    endif()
endfunction()

