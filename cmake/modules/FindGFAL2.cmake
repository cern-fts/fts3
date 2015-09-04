#
# This module detects if gfal2 is installed and determines where the
# include files and libraries are.
#
# This code sets the following variables:
# 
# GFAL2_LIBRARIES       = full path to the gfal2 libraries
# GFAL2_INCLUDE_DIR     = include dir to be used when using the gfal2 library
# GFAL2_FOUND           = set to true if gfal2 was found successfully
#
# GFAL2_LOCATION
#   setting this enables search for gfal2 libraries / headers in this location

find_package (PkgConfig)

pkg_check_modules(GFAL2_PKG gfal2)
pkg_check_modules(GFAL2_TRANSFER_PKG gfal_transfer)

if (GFAL2_PKG_FOUND AND GFAL2_TRANSFER_PKG_FOUND)
    set (GFAL2_LIBRARIES ${GFAL2_PKG_LIBRARIES} ${GFAL2_TRANSFER_PKG_LIBRARIES})
    set (GFAL2_INCLUDE_DIRS ${GFAL2_PKG_INCLUDE_DIRS} ${GFAL2_TRANSFER_PKG_INCLUDE_DIRS})
    set (GFAL2_DEFINITIONS "${GFAL2_PKG_CFLAGS} ${GFAL2_TRANSFER_PKG_CFLAGS}")
    set (GFAL2_LIBRARY_DIRS ${GFAL2_PKG_LIBRARY_DIRS})
else (GFAL2_PKG_FOUND AND GFAL2_TRANSFER_PKG_FOUND)

    find_library(GFAL2_CORE_LIBRARIES
        NAMES gfal2
        HINTS ${GFAL2_LOCATION} 
              ${CMAKE_INSTALL_PREFIX}/Grid/gfal2/*/${PLATFORM}/lib
              ${CMAKE_INSTALL_PREFIX}/Grid/gfal2/*/${PLATFORM}/lib64
        DOC "The main gfal2 library"
    )
    find_library(GFAL2_TRANSFER_LIBRARIES
        NAMES gfal_transfer
        HINTS ${GFAL2_LOCATION} 
              ${CMAKE_INSTALL_PREFIX}/Grid/gfal2/*/${PLATFORM}/lib
              ${CMAKE_INSTALL_PREFIX}/Grid/gfal2/*/${PLATFORM}/lib64
        DOC "The transfer gfal2 library"
    )
    set (GFAL2_LIBRARIES ${GFAL2_CORE_LIBRARIES} ${GFAL2_TRANSFER_LIBRARIES})

    find_path(GFAL2_INCLUDE_DIRS 
        NAMES gfal_api.h
        HINTS ${GFAL2_LOCATION}/include/*
              ${CMAKE_INSTALL_PREFIX}/Grid/gfal2/*/${PLATFORM}/include/*
        DOC "The gfal2 include directory"
    )

    set (GFAL2_DEFINITIONS "")
endif (GFAL2_PKG_FOUND AND GFAL2_TRANSFER_PKG_FOUND)

if (GFAL2_LIBRARIES)
    message (STATUS "GFAL2 libraries: ${GFAL2_LIBRARIES}")
endif (GFAL2_LIBRARIES)
if (GFAL2_INCLUDE_DIRS)
    message (STATUS "GFAL2 include dir: ${GFAL2_INCLUDE_DIRS}")
endif (GFAL2_INCLUDE_DIRS)

# -----------------------------------------------------
# handle the QUIETLY and REQUIRED arguments and set GFAL2_FOUND to TRUE if 
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args (GFAL2 DEFAULT_MSG
    GFAL2_LIBRARIES  GFAL2_INCLUDE_DIRS
)
mark_as_advanced(GFAL2_INCLUDE_DIRS GFAL2_LIBRARIES GFAL2_LIBRARY_DIRS)
