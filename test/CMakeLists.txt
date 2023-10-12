#
# Copyright (c) CERN 2015
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

find_package (Boost COMPONENTS unit_test_framework chrono)
find_package (GLIB2)

include_directories (${Boost_INCLUDE_DIRS})
include_directories (${GLIB2_INCLUDE_DIRS})
include_directories (${CMAKE_SOURCE_DIR}/src)
include_directories (${CMAKE_BINARY_DIR}/src)
include_directories (${CURL_INCLUDE_DIR})
# Tests
add_subdirectory (unit)
