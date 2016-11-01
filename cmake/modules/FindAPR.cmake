# - Try to find APR, the Apache Portable Runtime Library
# Once done this will define
#
#  APR_FOUND - System has APR
#  APR_INCLUDE_DIR - The APR include directory
#  APR_LIBRARIES - The libraries needed to use APR
#  APR_DEFINITIONS - Compiler switches required for using APR
#
#
# Setting the var
#  APR_ALTLOCATION
#  will enforce the search for the libraries/executables in a different place
#  with respect to the eventual system-wide setup

#=============================================================================
# By Fabrizio Furano (CERN IT/GT), Oct 2010


IF (APR_INCLUDE_DIR AND APR_LIBRARIES)
    # in cache already
    SET(APR_FIND_QUIETLY TRUE)
ENDIF (APR_INCLUDE_DIR AND APR_LIBRARIES)





IF (NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls

    if (NOT LIBDESTINATION)
        set(LIBDESTINATION "lib")
    endif()

    FIND_PACKAGE(PkgConfig)
    IF (APR_ALTLOCATION)
        set( ENV{PKG_CONFIG_PATH} "${APR_ALTLOCATION}/${LIBDESTINATION}/pkgconfig:${APRUTIL_ALTLOCATION}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}" )
        MESSAGE(STATUS "Looking for APR package in $ENV{PKG_CONFIG_PATH}")
    ENDIF()
    PKG_CHECK_MODULES(PC_APR apr-1)

    foreach(i ${PC_ACTIVEMQCPP_CFLAGS_OTHER})
        message(STATUS "${i}")
        SET(APR_DEFINITIONS "${APR_DEFINITIONS} ${i}")
    endforeach()

    foreach(i ${PC_ACTIVEMQCPP_LDFLAGS})
        message(STATUS "${i}")
        SET(APR_LINKDEFINITIONS "${APR_LINKDEFINITIONS} ${i}")
    endforeach()



    #  SET(APR_DEFINITIONS ${PC_APR_CFLAGS_OTHER})
    SET(APR_VERSION ${PC_APR_VERSION})
ENDIF (NOT WIN32)

IF (NOT APR_ALTLOCATION)
    SET(APR_ALTLOCATION "" CACHE PATH "An alternate forced location for APR")
ENDIF()

# Find the includes, by giving priority to APR_ALTLOCATION, if set
IF (APR_ALTLOCATION)
    find_path(APR_INCLUDE_DIR apr.h DOC "Path to the APR include file"
            HINTS
            ${APR_ALTLOCATION}
            ${APR_ALTLOCATION}/include
            ${APR_ALTLOCATION}/include/*
            NO_SYSTEM_ENVIRONMENT_PATH
            )
    IF(APR_INCLUDE_DIR)
        MESSAGE(STATUS "APR includes found in ${APR_INCLUDE_DIR}")
    else()
        MESSAGE(FATAL_ERROR "APR includes not found. You may want to suggest a location by using -DAPR_ALTLOCATION=<location of APR>")
    ENDIF()
ENDIF (APR_ALTLOCATION)

# Now look in the system wide places, if not yet found
find_path(APR_INCLUDE_DIR apr.h DOC "Path to the APR include file"
        HINTS
        ${PC_APR_INCLUDEDIR}
        ${PC_APR_INCLUDE_DIRS}
        ${PC_APR_INCLUDEDIR}/*
        )
IF(APR_INCLUDE_DIR)
    MESSAGE(STATUS "APR includes found in ${APR_INCLUDE_DIR}")

else()
    MESSAGE(FATAL_ERROR "APR includes not found. You may want to suggest a location by using -DAPR_ALTLOCATION=<location of APR>")
ENDIF()





# Find the libs, by giving priority to APR_ALTLOCATION, if set
IF (APR_ALTLOCATION)
    find_library(APR_LIBRARIES NAMES apr-1 DOC "Path to the apr-1 library"
            HINTS
            ${APR_ALTLOCATION}/lib
            NO_SYSTEM_ENVIRONMENT_PATH
            )
    IF(APR_LIBRARIES)
        MESSAGE(STATUS "apr-1 lib found in ${APR_LIBRARIES}")
    else()
        MESSAGE(FATAL_ERROR "apr-1 lib not found. You may want to suggest a location by using -DAPR_ALTLOCATION=<location of APR>")
    ENDIF()
ENDIF (APR_ALTLOCATION)

# Now look in the system wide places, if not yet found
FIND_LIBRARY(APR_LIBRARIES NAMES apr-1 DOC "Path to the apr-1 library"
        HINTS
        ${PC_APR_LIBDIR}
        ${PC_APR_LIBRARY_DIRS}
        )
IF(APR_LIBRARIES)
    MESSAGE(STATUS "apr-1 lib found in ${APR_LIBRARIES}")
else()
    MESSAGE(FATAL_ERROR "apr-1 lib not found. You may want to suggest a location by using -DAPR_ALTLOCATION=<location of APR>")
ENDIF()





#
# Version checking
#
if(APR_FIND_VERSION AND APR_VERSION)
    if(APR_FIND_VERSION_EXACT)
        if(NOT APR_VERSION VERSION_EQUAL ${APR_FIND_VERSION})
            MESSAGE(FATAL_ERROR "Found APR ${APR_VERSION} != ${APR_FIND_VERSION}")
        endif()
    else()
        # version is too low
        if(NOT APR_VERSION VERSION_EQUAL ${APR_FIND_VERSION} AND
                NOT APR_VERSION VERSION_GREATER ${APR_FIND_VERSION})
            MESSAGE(FATAL_ERROR "Found APR ${APR_VERSION} < ${APR_FIND_VERSION} (Minimum required). You may want to suggest a location by using -DAPR_ALTLOCATION=<location of APR>")
        endif()
    endif()
endif()






INCLUDE(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set APR_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(APR DEFAULT_MSG APR_LIBRARIES APR_INCLUDE_DIR)

MARK_AS_ADVANCED(APR_INCLUDE_DIR APR_LIBRARIES)
