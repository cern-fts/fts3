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
find_package(JSONCPP)
find_package(Cryptopp)
find_package(Davix)

# Sources
file(GLOB fts_token_SOURCES "*.cpp" "services/*.cpp" "executors/*.cpp")

add_executable(fts_token ${fts_token_SOURCES})

target_include_directories(fts_token PUBLIC
    ${GLIB2_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${CRYPTOPP_INCLUDE_DIRS}
    ${DAVIX_INCLUDE_DIR}
)

target_link_libraries(fts_token
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    ${GLIB2_LIBRARIES}
    ${Boost_LIBRARIES}
    fts_db_generic
    fts_config
    fts_common
    fts_msg_ifce
    fts_server_lib
    JSONCPP::JSONCPP
    ${CRYPTOPP_LIBRARIES}
    ${DAVIX_LIBRARIES}
)

# Install
install(TARGETS fts_token RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})

# Install Python token housekeeper script
install(FILES scripts/ftstokenhousekeeperd
        DESTINATION ${CMAKE_INSTALL_SBINDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ)

install(FILES scripts/ftstokenhousekeeperd.ini
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/fts3)
