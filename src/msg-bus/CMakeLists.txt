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

# Find boost
find_package (Boost COMPONENTS filesystem REQUIRED)
find_package (DIRQ REQUIRED)
find_package (GLIB2 REQUIRED)
find_package (Protobuf REQUIRED)

include_directories (${DIRQ_INCLUDE_DIR})
include_directories(${LIBUUID_INCLUDE_DIRS})

# Disable "-Wconversion" warning during Protobuf generation
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion")

# Protocol buffers
file (GLOB EVENTS "events/*.proto")

protobuf_generate_cpp (EVENTS_SOURCES EVENTS_HEADERS
    PROTOFILES
    ${EVENTS}
    PROTOROOT "${CMAKE_CURRENT_SOURCE_DIR}/events"
    OUTPATH "${CMAKE_CURRENT_BINARY_DIR}/events"
)

# Sources
file (GLOB msg_bus_sources "*.cpp")

add_library(fts_msg_bus SHARED
    ${msg_bus_sources}
    ${EVENTS_SOURCES} ${EVENTS_HEADERS}
)
target_link_libraries(fts_msg_bus
    ${Boost_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${DIRQ_LIBRARIES}
    ${PROTOBUF_LIBRARY}
    -lpthread
)

set_target_properties(fts_msg_bus PROPERTIES
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
)


# Install only if we are building the server or the client
# (Do not install if only building a database backend, for instance)
if(SERVERBUILD)
    install(TARGETS fts_msg_bus
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    )
endif ()
