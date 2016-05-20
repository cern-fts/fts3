#
# This module detects if dirq is installed and determines where the
# include files and libraries are.
#
# This code sets the following variables:
# 
# DIRQ_LIBRARIES       = full path to the dirq library
# DIRQ_INCLUDE_DIR     = include dir to be used when using the dirq library
# DIRQ_FOUND           = set to true if dirq was found successfully
#
# DIRQ_LOCATION
#   setting this enables search for dirq libraries / headers in this location

find_package (PkgConfig)
pkg_check_modules (DIRQ_PKG dirq)

if (DIRQ_PKG)
    set (DIRQ_LIBRARIES ${DIRQ_PKG_LIBRARIES})
    set (DIRQ_INCLUDE_DIR ${DIRQ_PKG_INCLUDE_DIRS})
    set (DIRQ_DEFINITIONS "${DIRQ_PKG_CFLAGS} ${DIRQ_PKG_CFLAGS_OTHER}")
    set (DIRQ_LIBRARY_DIRS ${DIRQ_PKG_LIBRARY_DIRS})
else (DIRQ_PKG)

    find_library(DIRQ_LIBRARIES
        NAMES dirq
        HINTS ${DIRQ_LOCATION}
        DOC "dirq library"
    )

    find_path(DIRQ_INCLUDE_DIR
        NAMES dirq.h
        HINTS
            ${DIRQ_LOCATION}
            ${DIRQ_LOCATION}/include/*
        DOC "dirq include directory"
    )

    set (DIRQ_DEFINITIONS "")
endif (DIRQ_PKG)

if (DIRQ_LIBRARIES)
    message (STATUS "DIRQ libraries: ${DIRQ_LIBRARIES}")
endif (DIRQ_LIBRARIES)
if (DIRQ_INCLUDE_DIR)
    message (STATUS "DIRQ include dir: ${DIRQ_INCLUDE_DIR}")
endif (DIRQ_INCLUDE_DIR)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    link_directories ("/usr/local/opt/gettext/lib")
endif ()

# -----------------------------------------------------
# handle the QUIETLY and REQUIRED arguments and set DIRQ_FOUND to TRUE if
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args (DIRQ DEFAULT_MSG
    DIRQ_LIBRARIES  DIRQ_INCLUDE_DIR
)
mark_as_advanced(DIRQ_INCLUDE_DIR DIRQ_LIBRARIES DIRQ_LIBRARY_DIRS)
