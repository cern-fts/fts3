#
# Copyright (c) CERN 2013-2025
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

file(GLOB fts_config "*.cpp")
add_library(fts_config SHARED ${fts_config})
find_package(Boost COMPONENTS program_options regex thread)

target_link_libraries(fts_config
    fts_common
    ${Boost_LIBRARIES}
)
set_target_properties(fts_config PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/src/config
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
)

# Artifacts
if(SERVERBUILD)
    install(TARGETS fts_config
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    install(FILES fts3config
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/fts3
        PERMISSIONS OWNER_READ OWNER_WRITE
    )

    install(FILES fts-activemq.conf
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/fts3
        PERMISSIONS OWNER_READ OWNER_WRITE )

endif()
