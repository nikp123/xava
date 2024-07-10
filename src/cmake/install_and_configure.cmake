# Generate README file
if(MSYS OR MSVC OR MINGW)
    configure_file("assets/windows/readme.txt" "README.txt" NEWLINE_STYLE CRLF)
endif()

# Generate proper license file
file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/final-LICENSE.txt")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/LICENSE" "${CMAKE_CURRENT_BINARY_DIR}/final-LICENSE.txt")

file(GLOB LICENSE_FILES "${CMAKE_CURRENT_BINARY_DIR}/LICENSE_*.txt")
foreach(LICENSE_FILE ${LICENSE_FILES})
    cat(${LICENSE_FILE} "${CMAKE_CURRENT_BINARY_DIR}/final-LICENSE.txt")
endforeach()

# Install
install (TARGETS xava DESTINATION bin)
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/final-LICENSE.txt" DESTINATION share/licenses/xava)
install (FILES example_files/config RENAME config.example DESTINATION share/xava)

# Install DLLs
find_and_copy_dlls(xava)

include("src/cmake/copy_gl_shaders.cmake")

if(UNIX AND NOT APPLE)
    # I am lazy and I just want to convert the icons into a format most desktops will be fine with
    find_program(CONVERT magick
                 REQUIRED)

    # Despite the program not being called, we need it because it's called by magick
    find_program(LIBRSVG rsvg-convert
                 REQUIRED)

    execute_process (COMMAND
        "${CONVERT}" convert -size 128x128 -density 1200 -background none -format png32
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/linux/xava.svg"
        "${CMAKE_CURRENT_BINARY_DIR}/xava.png"
         COMMAND_ERROR_IS_FATAL ANY)
    install (FILES assets/linux/xava.desktop DESTINATION share/applications)
    install (FILES assets/linux/xava.svg RENAME xava_visualizer.svg DESTINATION share/icons/hicolor/scalable/apps)
    install (FILES "${CMAKE_CURRENT_BINARY_DIR}/xava.png" RENAME xava_visualizer.png DESTINATION share/icons/hicolor/128x128/apps)
endif()
