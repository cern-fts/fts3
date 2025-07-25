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

# Find libuuid
find_package(LibUuid REQUIRED)
include_directories(${LIBUUID_INCLUDE_DIRS})

# Find boost
find_package(Boost COMPONENTS regex iostreams system thread filesystem REQUIRED)

# Sources
file(GLOB fts_common_sources "*.cpp")

add_library(fts_common SHARED ${fts_common_sources})
target_link_libraries(fts_common
    ${Boost_LIBRARIES}
    ${LIBUUID_LIBRARIES}
    ${GLIB2_LIBRARIES}
    -lpthread
)

set_target_properties(fts_common PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/common
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
)

# Unit tests
if(FTS3_COMPILE_WITH_UNITTEST)
    find_package(Boost COMPONENTS unit_test_framework REQUIRED)
	target_link_libraries(fts_common ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES} )
endif()

# Install only if we are building the server or the client
# (Do not install if only building a database backend, for instance)
if(SERVERBUILD OR CLIENTBUILD)
    install(TARGETS fts_common
        RUNTIME DESTINATION ${CMAKE_INSTALL_RUNTIME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif ()
