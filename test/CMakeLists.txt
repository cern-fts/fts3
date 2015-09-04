#
# Copyright (c) CERN 2013-2015
#
# Copyright (c) Members of the EMI Collaboration. 2010-2013
#  See  http://www.eu-emi.eu/partners for details on the copyright
#  holders.
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

cmake_minimum_required (VERSION 2.8)

project (fts3-tests)

if(NOT DEFINED SHARE_INSTALL_PREFIX)
  set(SHARE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/usr/share")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    install (PROGRAMS FTS3-config
             DESTINATION ${SHARE_INSTALL_PREFIX}/fts3/tests)
    
    install (DIRECTORY functional stress rest
             DESTINATION ${SHARE_INSTALL_PREFIX}/fts3/tests
             USE_SOURCE_PERMISSIONS)
endif()
