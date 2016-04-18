#
# This module detects if gsoap is installed and determines where the
# include files and libraries are.
#
# This code sets the following variables:
# 
# GSOAP_LIBRARIES       = full path to the gsoap libraries
# GSOAP_SSL_LIBRARIES   = full path to the gsoap ssl libraries
# GSOAP_INCLUDE_DIR     = include dir to be used when using the gsoap library
# GSOAP_WSDL2H          = wsdl2h binary
# GSOAP_SOAPCPP2        = soapcpp2 binary
# GSOAP_FOUND           = set to true if gsoap was found successfully
#
# GSOAP_LOCATION
#   setting this enables search for gsoap libraries / headers in this location


# ------------------------------------------------------
# try pkg config search
#
# -----------------------------------------------------


find_package(PkgConfig)
pkg_check_modules(PC_GSOAP gsoap)

IF(PC_GSOAP_FOUND)

    SET(GSOAP_LIBRARIES ${PC_GSOAP_LIBRARIES})
    SET(GSOAP_INCLUDE_DIR ${PC_GSOAP_INCLUDE_DIRS})
    if (NOT GSOAP_INCLUDE_DIR)
        set (GSOAP_INCLUDE_DIR "/usr/include")
    endif (NOT GSOAP_INCLUDE_DIR)
    SET(GSOAP_DEFINITIONS "${PC_GSOAP_CFLAGS} ${PC_GSOAP_CFLAGS_OTHER}")

ELSE(PC_GSOAP_FOUND)

    # -----------------------------------------------------
    # GSOAP Libraries
    # -----------------------------------------------------
    find_library(GSOAP_LIBRARIES
        NAMES gsoap
        HINTS ${GSOAP_LOCATION} 
              ${CMAKE_INSTALL_PREFIX}/gsoap/*/${PLATFORM}/
        DOC "The main gsoap library"
    )
    
    # -----------------------------------------------------
    # GSOAP Include Directories
    # -----------------------------------------------------
    find_path(GSOAP_INCLUDE_DIR 
        NAMES stdsoap2.h
        HINTS ${GSOAP_LOCATION}
              ${CMAKE_INSTALL_PREFIX}/gsoap/*/${PLATFORM}/
        DOC "The gsoap include directory"
    )
    
    SET(GSOAP_DEFINITIONS "")

ENDIF(PC_GSOAP_FOUND)

# -----------------------------------------------------
# GSOAP ssl Libraries
# -----------------------------------------------------

find_library(GSOAP_SSL_LIBRARIES
    NAMES gsoapssl
    HINTS ${GSOAP_LOCATION} 
          ${CMAKE_INSTALL_PREFIX}/gsoap/*/${PLATFORM}/
    DOC "The ssl gsoap library"
)

# -----------------------------------------------------
# GSOAP Binaries
# -----------------------------------------------------
find_program(GSOAP_WSDL2H
    NAMES wsdl2h
    HINTS ${GSOAP_LOCATION}/bin
          ${CMAKE_INSTALL_PREFIX}/gsoap/*/${PLATFORM}/bin/
    DOC "The gsoap bin directory"
)
find_program(GSOAP_SOAPCPP2
    NAMES soapcpp2
    HINTS ${GSOAP_LOCATION}/bin
          ${CMAKE_INSTALL_PREFIX}/gsoap/*/${PLATFORM}/bin/
    DOC "The gsoap bin directory"
)

# -----------------------------------------------------
# GSOAP_276_COMPAT_FLAGS and GSOAPVERSION
# try to determine the flagfor the 2.7.6 compatiblity, break with 2.7.13 and re-break with 2.7.16
# ----------------------------------------------------
message(STATUS " - wsdlh : ${GSOAP_WSDL2H}")
message(STATUS " - SOAPCPP2 : ${GSOAP_SOAPCPP2}")

# some versions of soapcpp2 interpret "-v" as verbose, and hang while waiting for input
# try "-help" first, and if it fails, do "-v"
execute_process(COMMAND ${GSOAP_SOAPCPP2}  "-help"   OUTPUT_VARIABLE GSOAP_STRING_VERSION ERROR_VARIABLE GSOAP_STRING_VERSION )
string(REGEX MATCH "[0-9]*\\.[0-9]*\\.[0-9]*" GSOAP_VERSION ${GSOAP_STRING_VERSION})

if( "${GSOAP_VERSION}" STREQUAL "..")
  execute_process(COMMAND ${GSOAP_SOAPCPP2}  "-v"   OUTPUT_VARIABLE GSOAP_STRING_VERSION ERROR_VARIABLE GSOAP_STRING_VERSION )
  string(REGEX MATCH "[0-9]*\\.[0-9]*\\.[0-9]*" GSOAP_VERSION ${GSOAP_STRING_VERSION})
endif()

message(STATUS " - GSOAP VERSION : ${GSOAP_VERSION}")
if( "${GSOAP_VERSION}"  VERSION_LESS "2.7.6")
	set(GSOAP_276_COMPAT_FLAGS "")
elseif ( "${GSOAP_VERSION}"  VERSION_LESS "2.7.14") 
	set(GSOAP_276_COMPAT_FLAGS "-z")	
else ( "${GSOAP_VERSION}"  VERSION_LESS "2.7.14") 
	set(GSOAP_276_COMPAT_FLAGS "-z1 -z2")	
endif ( "${GSOAP_VERSION}"  VERSION_LESS "2.7.6")



# -----------------------------------------------------
# handle the QUIETLY and REQUIRED arguments and set GSOAP_FOUND to TRUE if 
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gsoap DEFAULT_MSG GSOAP_LIBRARIES 
    GSOAP_INCLUDE_DIR GSOAP_WSDL2H GSOAP_SOAPCPP2)
mark_as_advanced(GSOAP_INCLUDE_DIR GSOAP_LIBRARIES GSOAP_DEFINITIONS GSOAP_WSDL2H GSOAP_SOAPCPP2)
