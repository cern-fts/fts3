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

find_package (Boost COMPONENTS chrono filesystem system REQUIRED)
find_package (GLIB2)

include_directories (${GLIB2_INCLUDE_DIRS})

set(CMAKE_INCLUDE_CURRENT_DIR ON)


set(fts_db_cleaner_SOURCES clean.cpp)
add_executable(fts_db_cleaner ${fts_db_cleaner_SOURCES})
target_link_libraries(fts_db_cleaner
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    fts_db_generic
    fts_config
    fts_common
    fts_msg_bus
    ${Boost_LIBRARIES}
)

# Artifacts
install(TARGETS fts_db_cleaner RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})
