# Correct CMAKE_INSTALL_PREFIX so that distros don't break
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Default installation path" FORCE)
endif()

# REQUIRE GIT to be present
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git AND NOT NIX_BUILDER)
    message(FATAL_ERROR "XAVA from 0.7.0 onwards requires to be built in a .git directory")
endif()

# Fix pkg-config for cross-builds (such as MinGW on ArchLinux)
if(CMAKE_FIND_ROOT_PATH)
    set(CMAKE_SYSROOT "${CMAKE_FIND_ROOT_PATH}")

    set(ENV{PKG_CONFIG_DIR} "")
    set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/lib/pkgconfig:${CMAKE_SYSROOT}/share/pkgconfig")
    set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
endif()

if(UNIX_INDEPENDENT_PATHS)
    add_definitions("-DUNIX_INDEPENDENT_PATHS")
endif()
