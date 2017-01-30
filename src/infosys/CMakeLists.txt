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

find_package (Boost COMPONENTS thread)


set(fts_infosys_sources
    BdiiBrowser.cpp
    SiteNameCacheRetriever.cpp
)

add_library(fts_infosys SHARED ${fts_infosys_sources})
target_link_libraries(fts_infosys
    fts_common
    fts_config
    ${Boost_LIBRARIES}
    -lldap_r
)

set_target_properties(fts_infosys PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/infosys
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
)

if(FTS3_COMPILE_WITH_UNITTEST)
	find_package (Boost COMPONENTS unit_test_framework REQUIRED)
	target_link_libraries(fts_infosys ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES})
endif()


# Artifacts
if(SERVERBUILD)
    install(TARGETS fts_infosys
    	RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
    	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    )
endif()
