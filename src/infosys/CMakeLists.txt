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
# See the License for the specific language governing permissions and 
# limitations under the License. 

cmake_minimum_required(VERSION 2.6)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(fts_include_dir 
	"${PROJECT_SOURCE_DIR}/src/"
	"${CMAKE_BINARY_DIR}/src/"
)

include_directories (${fts_include_dir})
include_directories (.)

file(GLOB fts_infosys_sources "*.cpp")

find_package (Boost COMPONENTS thread)

add_library(fts_infosys SHARED ${fts_infosys_sources})
target_link_libraries(fts_infosys ${Boost_LIBRARIES} )
target_link_libraries(fts_infosys -lldap_r 	-lpugixml)


set_target_properties(fts_infosys             PROPERTIES
                                                LIBRARY_OUTPUT_DIRECTORY
${PROJECT_BINARY_DIR}/src/infosys
                                                VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                                SOVERSION ${VERSION_MAJOR}
                                                CLEAN_DIRECT_OUTPUT 1)



if(FTS3_COMPILE_WITH_UNITTEST)
	target_link_libraries(fts_infosys -lboost_unit_test_framework-mt )
endif()


if (MAINBUILD STREQUAL "ON" OR ALLBUILD STREQUAL "ON")
install(TARGETS         fts_infosys 
	RUNTIME             DESTINATION ${CMAKE_INSTALL_PREFIX} 
	LIBRARY             DESTINATION ${LIB_INSTALL_DIR} )
endif ()
