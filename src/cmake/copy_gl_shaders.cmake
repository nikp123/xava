# Install vertex shaders
file(GLOB filters "example_files/gl/shaders/*/vertex.glsl" )
foreach(files ${filters})
    get_filename_component(path ${files} REALPATH DIRECTORY)
    string(REPLACE "${xava_SOURCE_DIR}/example_files/" "" target_path ${path})
    string(REPLACE "vertex.glsl" "" target_path ${target_path})
    configure_file("example_files/${target_path}/vertex.glsl" ${target_path}/vertex.glsl COPYONLY)
    install(FILES "example_files/${target_path}/vertex.glsl" RENAME vertex.glsl.example DESTINATION "share/xava/${target_path}")
endforeach()

# Install geometry shaders
file(GLOB filters "example_files/gl/shaders/*/geometry.glsl" )
foreach(files ${filters})
    get_filename_component(path ${files} REALPATH DIRECTORY)
    string(REPLACE "${xava_SOURCE_DIR}/example_files/" "" target_path ${path})
    string(REPLACE "geometry.glsl" "" target_path ${target_path})
    configure_file("example_files/${target_path}/geometry.glsl" ${target_path}/geometry.glsl COPYONLY)
    install(FILES "example_files/${target_path}/geometry.glsl" RENAME geometry.glsl.example DESTINATION "share/xava/${target_path}")
endforeach()

# Install fragment shaders
file(GLOB filters "example_files/gl/shaders/*/fragment.glsl" )
foreach(files ${filters})
    get_filename_component(path ${files} REALPATH DIRECTORY)
    string(REPLACE "${xava_SOURCE_DIR}/example_files/" "" target_path ${path})
    string(REPLACE "fragment.glsl" "" target_path ${target_path})
    configure_file("example_files/${target_path}/fragment.glsl" ${target_path}/fragment.glsl COPYONLY)
    install(FILES "example_files/${target_path}/fragment.glsl" RENAME fragment.glsl.example DESTINATION "share/xava/${target_path}")
endforeach()

# Install shader configs
file(GLOB filters "example_files/gl/shaders/*/config.ini" )
foreach(files ${filters})
    get_filename_component(path ${files} REALPATH DIRECTORY)
    string(REPLACE "${xava_SOURCE_DIR}/example_files/" "" target_path ${path})
    string(REPLACE "config.ini" "" target_path ${target_path})
    configure_file("example_files/${target_path}/config.ini" ${target_path}/config.ini COPYONLY)
    install(FILES "example_files/${target_path}/config.ini" RENAME config.ini.example DESTINATION "share/xava/${target_path}")
endforeach()

