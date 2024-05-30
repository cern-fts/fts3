#
# Copyright (c) CERN 2023
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
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This code sets the following variables:
#
# DAVIX_LIBRARIES   = full path to the Davix libraries
# DAVIX_INCLUDE_DIR = include dir to be used when using the Davix library
# DAVIX_FOUND       = set to true if Davix was found successfully
#
# DAVIX_LOCATION
#   setting this enables search for Davix libraries / headers in this location

# -----------------------------------------------------
# Try with pkgconfig first
# -----------------------------------------------------

pkg_check_modules(DAVIX_PKG davix>=0.7.6)
pkg_check_modules(DAVIX_COPY_PKG davix_copy>=0.7.6)

if (DAVIX_PKG_FOUND AND DAVIX_COPY_PKG_FOUND)
    set (DAVIX_INCLUDE_DIR "${DAVIX_PKG_INCLUDE_DIRS}" "${DAVIX_COPY_PKG_INCLUDE_DIRS}")
    set (DAVIX_LIBRARIES "${DAVIX_PKG_LIBRARIES}" "${DAVIX_COPY_PKG_LIBRARIES}")
    set (DAVIX_CFLAGS "${DAVIX_PKG_CFLAGS} ${DAVIX_COPY_PKG_FLAGS}")
else ()
    # Davix Libraries
    find_library(DAVIX_MAIN_LIBRARY
            NAMES davix
            HINTS
            ${DAVIX_LOCATION}/lib ${DAVIX_LOCATION}/lib64 ${DAVIX_LOCATION}/lib32
            ${STAGE_DIR}/lib ${STAGE_DIR}/lib64
            ${CMAKE_INSTALL_PREFIX}/Davix/*/${PLATFORM}/lib
            ${CMAKE_INSTALL_PREFIX}/Davix/*/${PLATFORM}/lib64
            ${CMAKE_INSTALL_PREFIX}/lib
            DOC "The main davix library"
            )

    find_library(DAVIX_COPY_LIBRARY
            NAMES davix_copy
            HINTS
            ${DAVIX_LOCATION}/lib ${DAVIX_LOCATION}/lib64 ${DAVIX_LOCATION}/lib32
            ${STAGE_DIR}/lib ${STAGE_DIR}/lib64
            ${CMAKE_INSTALL_PREFIX}/Davix/*/${PLATFORM}/lib
            ${CMAKE_INSTALL_PREFIX}/Davix/*/${PLATFORM}/lib64
            ${CMAKE_INSTALL_PREFIX}/lib
            DOC "The davix copy library"
            )

    set (DAVIX_LIBRARIES ${DAVIX_MAIN_LIBRARY} ${DAVIX_COPY_LIBRARY})

    # Davix Include Directories
    find_path(DAVIX_INCLUDE_DIR
            NAMES davix.hpp
            HINTS
            ${DAVIX_LOCATION} ${DAVIX_LOCATION}/include ${DAVIX_LOCATION}/include/*
            ${STAGE_DIR}/include ${STAGE_DIR}/include
            ${CMAKE_INSTALL_PREFIX}/Davix/*/${PLATFORM}/include/*
            ${CMAKE_INSTALL_PREFIX}/include/davix
            DOC "Davix include directory"
            )

    set (DAVIX_CFLAGS "")
endif()

if (DAVIX_LIBRARIES)
    message (STATUS "DAVIX libraries: ${DAVIX_LIBRARIES}")
endif (DAVIX_LIBRARIES)
if(DAVIX_INCLUDE_DIR)
    message(STATUS "DAVIX includes found in ${DAVIX_INCLUDE_DIR}")
endif()

# -----------------------------------------------------
# handle the QUIETLY and REQUIRED arguments and set DAVIX_FOUND to TRUE if
# all listed variables are TRUE
# -----------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Davix DEFAULT_MSG DAVIX_LIBRARIES DAVIX_INCLUDE_DIR)
mark_as_advanced(DAVIX_LIBRARIES DAVIX_INCLUDE_DIR DAVIX_CFLAGS)
