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

file(GLOB sources "*.cpp")
file(GLOB cli_tests "${PROJECT_SOURCE_DIR}/src/cli/tests/*.cpp")

add_executable (fts_unittest 
	${sources} 
	${cli_tests}
)

SET(fts_libs 
    fts_config 
    fts_common 
    fts_cli_common
)

target_link_libraries(fts_unittest -lboost_unit_test_framework ${fts_libs})

install(TARGETS fts_unittest 
	RUNTIME DESTINATION ${BIN_INSTALL_DIR}
)
