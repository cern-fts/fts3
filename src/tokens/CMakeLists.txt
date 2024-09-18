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
file(GLOB fts_token_SOURCES "*.cpp" "services/*.cpp")

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
install(TARGETS fts_token RUNTIME
        DESTINATION ${SBIN_INSTALL_DIR})

# Install token Python scripts
install(FILES ftstokenhousekeeperd ftstokenrefresherd
        DESTINATION ${SBIN_INSTALL_DIR})

install(FILES ftstokenhousekeeperd.ini ftstokenrefresherd.ini
        DESTINATION ${SYSCONF_INSTALL_DIR}/fts3)

if (SYSTEMD_INSTALL_DIR)
    install(FILES ftstokenhousekeeperd.service ftstokenrefresherd.service
            DESTINATION ${SYSTEMD_INSTALL_DIR})
endif ()
