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
    install (FILES ${CMAKE_BINARY_DIR}/xava.desktop DESTINATION share/applications)
    install (FILES assets/linux/xava.svg DESTINATION share/icons/hicolor/scalable/apps)
endif()
