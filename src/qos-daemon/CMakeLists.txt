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

# Dependencies
find_package(GFAL2)
find_package(GLIB2)
find_package(Boost COMPONENTS thread system filesystem)

include_directories(
    ${GFAL2_INCLUDE_DIRS}
    ${GLIB2_INCLUDE_DIRS}
)

# Sources
file(GLOB fts_qos_SOURCES "*.cpp" "task/*.cpp" "context/*.cpp" "fetch/*.cpp")

add_executable(fts_qos ${fts_qos_SOURCES})

target_link_libraries(fts_qos
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    ${GFAL2_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${Boost_LIBRARIES}
    fts_db_generic
    fts_config
    fts_common
	  fts_msg_bus
    fts_proxy
    fts_msg_ifce
	  fts_server_lib
)

# Install artifacts
install(TARGETS fts_qos RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})
