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
# CRYPTOPP_LIBRARIES       = full path to the cryptopp libraries
# CRYPTOPP_INCLUDE_DIRS    = include dir to be used when using the cryptopp library
# CRYPTOPP_FOUND           = set to true if cryptopp was found successfully

# -----------------------------------------------------
# Cryptopp Libraries
# -----------------------------------------------------
find_library(CRYPTOPP_LIBRARIES
        NAMES cryptopp
        HINTS ${CRYPTOPP_LOCATION}
        /lib /lib64 /usr/lib /usr/lib64
        DOC "The cryptopp library"
        )

# -----------------------------------------------------
# Cryptopp Include Directories
# -----------------------------------------------------
find_path(CRYPTOPP_INCLUDE_DIRS
        NAMES base64.h                # header for base64 encoding, not processor architecture
        HINTS ${CRYPTOPP_LOCATION}
        /usr/include/cryptopp
        DOC "The cryptopp headers"
        )

# -------------------------------------------------------------------
# Handle the QUIETLY and REQUIRED arguments
# and set CRYPTOPP to TRUE if all listed variables are TRUE
# -------------------------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(cryptopp DEFAULT_MSG
        CRYPTOPP_LIBRARIES CRYPTOPP_INCLUDE_DIRS)
mark_as_advanced(CRYPTOPP_LIBRARIES CRYPTOPP_INCLUDE_DIRS)
