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

function usage
{
    echo
    echo "Usage:"
    echo
    echo "$0 <username> <noask>"
    echo
    echo -e "  \033[1musername\033[0m: Your cern user name. It may be different, but it may cause"
    echo "  complications."
    echo
    echo -e "  \033[1mnoask\033[0m: Optional. If it is set, the script will not ask for key presses. "
    echo "  It means that you know what is going on and why."
    echo
    echo "Example: "
    echo
    echo "  $0 obamab 1"
    echo

}

# Ubuntu machine
function ubuntu_install_tools {
	apt-get install subversion-tools doxygen davfs2
}

function ubuntu_setup_tools {
     sudo chmod +s `which mount.davfs`
}

CMD_PATH=$(dirname $0)
source $CMD_PATH/fts3-utils.sh

# Check if command was used with the proper parameters and environment
USER=$1
NOASK=$2

# Check the number of command line parameters
if [ $# -eq 0 -o $# -gt 2 ]; then
    usage
    exit -1
fi

# We should nod develop under root. Connect your CERN account to the dev. environment. USER should
# be your CERN account name.
adduser $USER

echo
echo "visudo will be executed. Add a line for your user name \"$USER\", according to your password"
echo "policy. More info:"
echo
echo "   man visudo"
echo "   man sudoers"
echo
echo "Example line: su requires no password for user $USER:"
echo
echo "    $USER      ALL=(ALL)       NOPASSWD: ALL"
echo

HOME=/home/$USER
fts3_utils_wait_for_key $NOASK
visudo

echo "Installing the required software..."
echo

# Determine platform 

fts3_utils_check_platform

if [$FTS3_PLATFORM == "ubuntu"]; then
    ubuntu_install_tools 
    ubuntu_setup_tools 
fi




