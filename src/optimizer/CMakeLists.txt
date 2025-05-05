#
# Copyright (c) CERN 2024
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
find_package(GLIB2)
find_package(Boost COMPONENTS thread system)

 # Sources
file(GLOB fts_optimizer_SOURCES "*.cpp" "services/*.cpp")

add_executable(fts_optimizer ${fts_optimizer_SOURCES})

target_include_directories(fts_optimizer PUBLIC
    ${GLIB2_INCLUDE_DIRS}
)

target_link_libraries(fts_optimizer
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    ${GLIB2_LIBRARIES}
    ${Boost_LIBRARIES}
    fts_db_generic
    fts_config
    fts_common
    fts_msg_ifce
    fts_server_lib
)

# Install
install(TARGETS fts_optimizer RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})
