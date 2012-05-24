# - Try to find APR-util, the Apache Portable Runtime Library Utils
# Once done this will define
#
#  APRUTIL_FOUND - System has APRUTIL
#  APRUTIL_INCLUDE_DIR - The APRUTIL include directory
#  APRUTIL_LIBRARIES - The libraries needed to use APRUTIL
#  APRUTIL_DEFINITIONS - Compiler switches required for using APRUTIL
#  APRUTIL_LINKDEFINITIONS - Linker switches required for using APRUTIL
#
#
# Setting the var
#  APRUTIL_ALTLOCATION
#  will enforce the search for the libraries/executables in a different place
#  with respect to the eventual system-wide setup

#=============================================================================
# By Fabrizio Furano (CERN IT/GT), Oct 2010


IF (APRUTIL_INCLUDE_DIR AND APRUTIL_LIBRARIES)
  # in cache already
  SET(APRUTIL_FIND_QUIETLY TRUE)
ENDIF (APRUTIL_INCLUDE_DIR AND APRUTIL_LIBRARIES)

IF (NOT WIN32)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls

  if (NOT LIBDESTINATION)
    set(LIBDESTINATION "lib")
  endif()



  FIND_PACKAGE(PkgConfig)
  IF (APRUTIL_ALTLOCATION)
    set( ENV{PKG_CONFIG_PATH} "${APRUTIL_ALTLOCATION}/${LIBDESTINATION}/pkgconfig:${APRUTIL_ALTLOCATION}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}" )
    MESSAGE(STATUS "Looking for APRUTIL package in $ENV{PKG_CONFIG_PATH}")
  ENDIF()
  PKG_CHECK_MODULES(PC_APRUTIL apr-util-1)

  foreach(i ${PC_APRUTIL_CFLAGS_OTHER})
    #message(STATUS "${i}")
    SET(APRUTIL_DEFINITIONS "${APRUTIL_DEFINITIONS} ${i}")
  endforeach()

  foreach(i ${PC_APRUTIL_LDFLAGS})
    #message(STATUS "${i}")
    SET(APRUTIL_LINKDEFINITIONS "${APRUTIL_LINKDEFINITIONS} ${i}")
  endforeach()



#  SET(APRUTIL_DEFINITIONS ${PC_APRUTIL_CFLAGS_OTHER})
  SET(APRUTIL_VERSION ${PC_APRUTIL_VERSION})
ENDIF (NOT WIN32)

IF (NOT APRUTIL_ALTLOCATION)
  SET(APRUTIL_ALTLOCATION "" CACHE PATH "An alternate forced location for APRUTIL")
ENDIF()

# Find the includes, by giving priority to APRUTIL_ALTLOCATION, if set
IF (APRUTIL_ALTLOCATION)
  find_path(APRUTIL_INCLUDE_DIR apr_date.h DOC "Path to the APRUTIL include files"
    HINTS
    ${APRUTIL_ALTLOCATION}
    ${APRUTIL_ALTLOCATION}/include
    ${APRUTIL_ALTLOCATION}/include/*
    NO_SYSTEM_ENVIRONMENT_PATH
    )
  IF(APRUTIL_INCLUDE_DIR)
    MESSAGE(STATUS "APRUTIL includes found in ${APRUTIL_INCLUDE_DIR}")
  else()
    MESSAGE(FATAL_ERROR "APRUTIL includes not found. You may want to suggest a location by using -DAPRUTIL_ALTLOCATION=<location of APRUTIL>")
  ENDIF()
ENDIF (APRUTIL_ALTLOCATION)

# Now look in the system wide places, if not yet found
find_path(APRUTIL_INCLUDE_DIR apr.h DOC "Path to the APRUTIL include file"
  HINTS
  ${PC_APRUTIL_INCLUDEDIR}
  ${PC_APRUTIL_INCLUDE_DIRS}
  ${PC_APRUTIL_INCLUDEDIR}/*
  )
IF(APRUTIL_INCLUDE_DIR)
  MESSAGE(STATUS "APRUTIL includes found in ${APRUTIL_INCLUDE_DIR}")
  
else()
  MESSAGE(FATAL_ERROR "APRUTIL includes not found. You may want to suggest a location by using -DAPRUTIL_ALTLOCATION=<location of APRUTIL>")
ENDIF()





# Find the libs, by giving priority to APR_ALTLOCATION, if set
IF (APRUTIL_ALTLOCATION)
  find_library(APRUTIL_LIBRARIES NAMES aprutil-1 DOC "Path to the aprutil-1 library"
    HINTS
    ${APRUTIL_ALTLOCATION}/lib
    NO_SYSTEM_ENVIRONMENT_PATH
    )
  IF(APRUTIL_LIBRARIES)
    MESSAGE(STATUS "aprutil-1 lib found in ${APRUTIL_LIBRARIES}")
  else()
    MESSAGE(FATAL_ERROR "aprutil-1 lib not found. You may want to suggest a location by using -DAPRUTIL_ALTLOCATION=<location of APRUTIL>")
  ENDIF()
ENDIF (APRUTIL_ALTLOCATION)

# Now look in the system wide places, if not yet found
FIND_LIBRARY(APRUTIL_LIBRARIES NAMES aprutil-1 DOC "Path to the aprutil-1 library"
  HINTS
  ${PC_APRUTIL_LIBDIR}
  ${PC_APRUTIL_LIBRARY_DIRS}
  )
IF(APRUTIL_LIBRARIES)
  MESSAGE(STATUS "aprutil-1 lib found in ${APRUTIL_LIBRARIES}")
else()
  MESSAGE(FATAL_ERROR "aprutil-1 lib not found. You may want to suggest a location by using -DAPRUTIL_ALTLOCATION=<location of APRUTIL>")
ENDIF()





#
# Version checking
#
if(APRUTIL_FIND_VERSION AND APRUTIL_VERSION)
    if(APRUTIL_FIND_VERSION_EXACT)
        if(NOT APRUTIL_VERSION VERSION_EQUAL ${APRUTIL_FIND_VERSION})
	  MESSAGE(FATAL_ERROR "Found APRUTIL ${APRUTIL_VERSION} != ${APRUTIL_FIND_VERSION}")
        endif()
    else()
        # version is too low
        if(NOT APRUTIL_VERSION VERSION_EQUAL ${APRUTIL_FIND_VERSION} AND 
                NOT APRUTIL_VERSION VERSION_GREATER ${APRUTIL_FIND_VERSION})
	      MESSAGE(FATAL_ERROR "Found APRUTIL ${APRUTIL_VERSION} < ${APRUTIL_FIND_VERSION} (Minimum required). You may want to suggest a location by using -DAPRUTIL_ALTLOCATION=<location of APRUTIL>")
        endif()
    endif()
endif()






INCLUDE(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set APR_FOUND to TRUE if 
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(APRUTIL DEFAULT_MSG APRUTIL_LIBRARIES APRUTIL_INCLUDE_DIR)

MARK_AS_ADVANCED(APRUTIL_INCLUDE_DIR APRUTIL_LIBRARIES)

