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

set(fts_db_generic_SOURCES SingleDbInstance.cpp DynamicLibraryManager.cpp DynamicLibraryManagerException.cpp)

add_library(fts_db_generic SHARED ${fts_db_generic_SOURCES})
target_link_libraries(fts_db_generic
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    fts_common
    fts_config
)
set_target_properties(fts_db_generic PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/db/generic
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
)

# Artifacts
# Required when building database backends, but install only when the server
# is built
if(SERVERBUILD)
    install(TARGETS fts_db_generic
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
endif()
