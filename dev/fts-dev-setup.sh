#! /bin/bash

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Version info: $Id: Makefile.am,v 1.2 2009/10/08 15:32:39 molnarzs Exp $
# Release: $Name:  $

## Setup a development machine: install development tools.
# Execute this script as root (su).

# ================================================================================
# Process command line parameters

function usage
{
    echo
    echo "Usage:"
    echo
    echo "$0 <username> <noask>"
    echo
    echo -e "  \033[1musername\033[0m: Your cern user name."
    echo
    echo -e "  \033[1mnoask\033[0m: Optional. If it is set, the script will not ask for key presses. "
    echo "  It means that you know what is going on and why."
    echo
    echo "Example: "
    echo
    echo "  $0 obamab 1"
    echo

}

# Check the number of command line parameters
if [ $# -eq 0 -o $# -gt 2 ]; then
    usage
    exit -1
fi

# ================================================================================
# Set up parameters 

PARAMETER_USER=$1
PARAMETER_NOASK=$2
FTS3_USERNAME=fts3

CMD_PATH=$(dirname $0)
source ${CMD_PATH}/fts-dev-setup-common.sh 

# ================================================================================
# Check platform support

fts_dev_setup_check_platform

PLATFORM_TOOLS=${CMD_PATH}/${PLATFORM}_tools.sh 

if [ -e $PLATFORM_TOOLS ]; then
    source $PLATFORM_TOOLS 
else
    echo
    echo -e "\033[1m$PLATFORM is not supported\033[0m ($PLATFORM_TOOLS file cannot be found)."
    exit -1
fi

# ================================================================================
# Get the setup functions

source $PLATFORM_TOOLS 

# ================================================================================

fts_dev_setup_common_user
fts_dev_setup_common_home_dir $PARAMETER_USER
fts_dev_setup_common_grid_credentials
fts_dev_setup_common_ssh_keys
fts_dev_setup_common_personal_settings
#fts_dev_setup_install_tools
fts_dev_setup_common_finalize $FTS3_USERNAME

