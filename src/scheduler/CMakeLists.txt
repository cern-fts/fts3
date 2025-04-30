#
# Copyright (c) CERN 2025
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

# Install Python package
install(DIRECTORY python/fts_scheduler
        DESTINATION /opt/fts
        DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
        FILES_MATCHING PATTERN "*.py")

# Install the configuration file
install(FILES fts-scheduler.ini
        DESTINATION ${SYSCONF_INSTALL_DIR}/fts3
        PERMISSIONS OWNER_READ OWNER_WRITE)

