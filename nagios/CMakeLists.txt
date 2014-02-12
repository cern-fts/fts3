cmake_minimum_required(VERSION 2.8)

project(nagios-plugins-fts)

# architecture detection
if(UNIX AND NOT WIN32)
    if(CMAKE_SIZEOF_VOID_P MATCHES 4)
          set (LIB_SUFFIX "")
          set (PKG_ARCH "i386")
    else(CMAKE_SIZEOF_VOID_P MATCHES 4)
          set (LIB_SUFFIX 64)
          set (PKG_ARCH "x86_64")
    endif(CMAKE_SIZEOF_VOID_P MATCHES 4)
endif(UNIX AND NOT WIN32)

add_subdirectory (config)
add_subdirectory (plugins)

