#
# This module detects if glib2 is installed and determines where the
# include files and libraries are.
#
# This code sets the following variables:
# 
# GLIB2_LIBRARIES       = full path to the glib2 libraries
# GLIB2_INCLUDE_DIR     = include dir to be used when using the glib2 library
# GLIB2_FOUND           = set to true if glib2 was found successfully
#
# GLIB2_LOCATION
#   setting this enables search for glib2 libraries / headers in this location

find_package (PkgConfig)
pkg_check_modules (GLIB2_PKG glib-2.0)

if (GLIB2_PKG_FOUND)
    set (GLIB2_LIBRARIES ${GLIB2_PKG_LIBRARIES})
    set (GLIB2_INCLUDE_DIRS ${GLIB2_PKG_INCLUDE_DIRS})
    set (GLIB2_DEFINITIONS "${GLIB2_PKG_CFLAGS} ${GLIB2_PKG_CFLAGS_OTHER}")
    set (GLIB2_LIBRARY_DIRS ${GLIB2_PKG_LIBRARY_DIRS})
else (GLIB2_PKG_FOUND)

    find_library(GLIB2_LIBRARIES
        NAMES libglib-2.0
        HINTS ${GLIB2_LOCATION} 
              ${CMAKE_INSTALL_PREFIX}/glib2/*/${PLATFORM}/
        DOC "The main glib2 library"
    )

    find_path(GLIB2_INCLUDE_DIRS 
        NAMES glib.h
        HINTS ${GLIB2_LOCATION}/include/*
              ${CMAKE_INSTALL_PREFIX}/glib2/*/${PLATFORM}/
        DOC "The glib2 include directory"
    )

    set (GLIB2_DEFINITIONS "")
endif (GLIB2_PKG_FOUND)

if (GLIB2_LIBRARIES)
    message (STATUS "GLIB2 libraries: ${GLIB2_LIBRARIES}")
endif (GLIB2_LIBRARIES)
if (GLIB2_INCLUDE_DIRS)
    message (STATUS "GLIB2 include dir: ${GLIB2_INCLUDE_DIRS}")
endif (GLIB2_INCLUDE_DIRS)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    link_directories ("/usr/local/opt/gettext/lib")
endif ()

# -----------------------------------------------------
# handle the QUIETLY and REQUIRED arguments and set GLIB2_FOUND to TRUE if 
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args (GLIB2 DEFAULT_MSG
    GLIB2_LIBRARIES  GLIB2_INCLUDE_DIRS
)
mark_as_advanced(GLIB2_INCLUDE_DIRS GLIB2_LIBRARIES GLIB2_LIBRARY_DIRS)

