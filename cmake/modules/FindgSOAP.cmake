# Copyright @ Members of the EMI Collaboration, 2010.
# See www.eu-emi.eu for details on the copyright holders.
# 
# Licensed under the Apache License, Version 2.0 (the "License"); 
# you may not use this file except in compliance with the License. 
# You may obtain a copy of the License at 
#
#    http://www.apache.org/licenses/LICENSE-2.0 
#
# Unless required by applicable law or agreed to in writing, software 
# distributed under the License is distributed on an "AS IS" BASIS, 
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
# See the License for the specific
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


# -----------------------------------------------------
# GSOAP Libraries
# -----------------------------------------------------
find_library(GSOAP_LIBRARIES
    NAMES gsoap++
    HINTS ${GSOAP_LOCATION}/lib ${GSOAP_LOCATION}/lib64 
          ${GSOAP_LOCATION}/lib32
    DOC "The main gsoap library"
)
find_library(GSOAP_SSL_LIBRARIES
    NAMES gsoapssl
    HINTS ${GSOAP_LOCATION}/lib ${GSOAP_LOCATION}/lib64 
          ${GSOAP_LOCATION}/lib32
    DOC "The ssl gsoap library"
)

# -----------------------------------------------------
# GSOAP Include Directories
# -----------------------------------------------------
find_path(GSOAP_INCLUDE_DIR 
    NAMES stdsoap2.h
    HINTS ${GSOAP_LOCATION} ${GSOAP_LOCATION}/include ${GSOAP_LOCATION}/include/*
    DOC "The gsoap include directory"
)

# -----------------------------------------------------
# GSOAP Import Include Directory
# -----------------------------------------------------
find_path(GSOAP_IMPORT_DIR 
    NAMES stlvector.h
    HINTS /usr/include/gsoap /usr/share/gsoap/import
    DOC "The gsoap include directory"
)

# -----------------------------------------------------
# GSOAP Binaries
# -----------------------------------------------------
find_program(GSOAP_WSDL2H
    NAMES wsdl2h
    HINTS ${GSOAP_LOCATION}/bin
    DOC "The gsoap bin directory"
)
find_program(GSOAP_SOAPCPP2
    NAMES soapcpp2
    HINTS ${GSOAP_LOCATION}/bin
    DOC "The gsoap bin directory"
)

# -----------------------------------------------------
# GSOAP_276_COMPAT_FLAGS and GSOAPVERSION
# try to determine the flagfor the 2.7.6 compatiblity, break with 2.7.13 and re-break with 2.7.16
# ----------------------------------------------------
message(STATUS " - wsdlh : ${GSOAP_WSDL2H}")
message(STATUS " - SOAPCPP2 : ${GSOAP_SOAPCPP2}")

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
# GSOAP C / C++ flags
# ----------------------------------------------------
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set (GSOAP_CXX_FLAGS "-DSOAP_DEBUG -DSOAP_MEM_DEBUG")
endif ()

# -----------------------------------------------------
# handle the QUIETLY and REQUIRED arguments and set GSOAP_FOUND to TRUE if 
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gsoap DEFAULT_MSG GSOAP_LIBRARIES 
    GSOAP_INCLUDE_DIR GSOAP_WSDL2H GSOAP_SOAPCPP2)
mark_as_advanced(GSOAP_INCLUDE_DIR GSOAP_LIBRARIES GSOAP_WSDL2H GSOAP_SOAPCPP2)
