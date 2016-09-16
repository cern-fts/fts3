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

cmake_minimum_required(VERSION 2.8)

# Set this directory as an include, since it is the root for the source
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package (GLIB2 REQUIRED)
include_directories(${GLIB2_INCLUDE_DIRS})

# Required by bdii-cache-update and conditionally by infosys
find_package(PugiXML)
if(NOT PUGIXML_FOUND)
    add_definitions(-DWITHOUT_PUGI)
endif(NOT PUGIXML_FOUND)

# Used by all submodules
add_subdirectory(common)

# Used by DB backends and server side
if(MYSQLBUILD OR SERVERBUILD)
    add_subdirectory(msg-bus)
    add_subdirectory(config)
    add_subdirectory(db)
endif()

# Used only by the server
if(SERVERBUILD)
    add_subdirectory(infosys)
    add_subdirectory(cred)
    add_subdirectory(server)
    add_subdirectory(bringonline-daemon)
    add_subdirectory(monitoring)
    add_subdirectory(url-copy)
    add_subdirectory(scripts)
    add_subdirectory(dbClear)
    add_subdirectory(glue2-publisher)
    add_subdirectory(myosg-update)

    if(PUGIXML_FOUND)
        add_subdirectory(bdii-cache-update)
    endif(PUGIXML_FOUND)
endif()

# Used only by the client
if(CLIENTBUILD)
    add_subdirectory(cli)
endif()
