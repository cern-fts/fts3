#
# This module detects if zeromq is installed and determines where the
# include files and libraries are.
#
# This code sets the following variables:
#
# ZMQ_LIBRARIES       = full path to the zeromq library
# ZMQ_INCLUDE_DIR     = include dir to be used when using the zeromq library
# ZMQ_FOUND           = set to true if zeromq was found successfully
#
# ZMQ_LOCATION
#   setting this enables search for zeromq libraries / headers in this location

find_package (PkgConfig)
pkg_check_modules (ZMQ_PKG zmq)

if (ZMQ_PKG)
    set (ZMQ_LIBRARIES ${ZMQ_PKG_LIBRARIES})
    set (ZMQ_INCLUDE_DIR ${ZMQ_PKG_INCLUDE_DIRS})
    set (ZMQ_DEFINITIONS "${ZMQ_PKG_CFLAGS} ${ZMQ_PKG_CFLAGS_OTHER}")
    set (ZMQ_LIBRARY_DIRS ${ZMQ_PKG_LIBRARY_DIRS})
else (ZMQ_PKG)

    find_library(ZMQ_LIBRARIES
        NAMES zmq
        HINTS ${ZMQ_LOCATION}
        DOC "zeromq library"
    )

    find_path(ZMQ_INCLUDE_DIR
        NAMES zmq.hpp
        HINTS
            ${ZMQ_LOCATION}
            ${ZMQ_LOCATION}/include/*
        DOC "zeromq include directory"
    )

    set (ZMQ_DEFINITIONS "")
endif (ZMQ_PKG)

if (ZMQ_LIBRARIES)
    message (STATUS "ZMQ libraries: ${ZMQ_LIBRARIES}")
endif (ZMQ_LIBRARIES)
if (ZMQ_INCLUDE_DIR)
    message (STATUS "ZMQ include dir: ${ZMQ_INCLUDE_DIR}")
endif (ZMQ_INCLUDE_DIR)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    link_directories ("/usr/local/opt/gettext/lib")
endif ()

# -----------------------------------------------------
# handle the QUIETLY and REQUIRED arguments and set ZMQ_FOUND to TRUE if
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args (ZMQ DEFAULT_MSG
    ZMQ_LIBRARIES  ZMQ_INCLUDE_DIR
)
mark_as_advanced(ZMQ_INCLUDE_DIR ZMQ_LIBRARIES ZMQ_LIBRARY_DIRS)
