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

function fts_dev_setup_common_user
{
    adduser $FTS3_USERNAME

    echo
    echo "visudo will be executed. Add a line for '$FTS3_USERNAME' user, according to your password"
    echo "policy. More info:"
    echo
    echo "   man visudo"
    echo "   man sudoers"
    echo
    echo "Example line: su requires no password for user $:"
    echo
    echo "    $FTS3_USERNAME      ALL=(ALL)       NOPASSWD: ALL"
    echo

    HOME=/home/$FTS3_USERNAME
    fts_dev_setup_wait_for_key $PARAMETER_NOASK
    visudo
}

# ================================================================================

function fts_dev_setup_check_if_variable_defined
{
    local VAR_NAME=$1
    local VAR_DEFAULT=$2
    local VAR_VALUE=$3

    # color codes, color howto: 
    # http://webhome.csc.uvic.ca/~sae/seng265/fall04/tips/s265s047-tips/bash-using-colors.html
    if [ ! -n "$VAR_VALUE" ] ;then
        echo
        echo -e "\e[1;31mERROR: $VAR_NAME is not set.\e[0m For example:"
        echo
        echo "export $VAR_NAME=$VAR_DEFAULT"
        echo
        return 1
    fi

    return 0
}

# ================================================================================

function fts_dev_setup_wrong_platform_error
{
    echo
    echo "PLATFORM environment variable must be set. It must be one of the supported"
    echo "platforms (see FTS3 wiki)."
    echo 

    exit -1
}  

# ================================================================================

function fts_dev_setup_check_platform
{
    fts_dev_setup_check_if_variable_defined PLATFORM "sl6_x86_64_gcc446EPEL" $PLATFORM

    if [ ! $? -eq 0 ]; then
        fts_dev_setup_wrong_platform_error
        exit -1
    fi
}

# ================================================================================

function fts_dev_setup_wait_for_key
{
    if [ ! -n "$PARAMETER_NOASK" ]; then
        echo "Press a key if ready to continue."
        read foo
    fi
}

# ================================================================================

function fts_dev_setup_common_home_dir
{
    echo
    echo "Setting up $HOME dir..."
    echo
    local USER=$1
    ln -s /afs/cern.ch/user/${USER:0:1}/$USER $HOME/afs
    echo "source \$HOME/.bashrc_fts3" >> $HOME/.bashrc
    echo "export PLATFORM=$PLATFORM" > $HOME/.bashrc_fts3
    echo "export PATH=$HOME/afs/bin:\$PATH" >> $HOME/.bashrc_fts3
    echo "export EDITOR=vim" >> $HOME/.bashrc_fts3
    echo "alias ls=\"ls --color\"" >> $HOME/.bashrc_fts3
}

# ================================================================================

function fts_dev_setup_common_grid_credentials
{
    echo
    echo "I need your CERN AFS password to continue (principal: $PARAMETER_USER)."
    echo "I will copy your credentials."
    echo "Ensure that .glite and .globus are OK in your AFS root."
    echo
    fts_dev_setup_wait_for_key
    kinit $PARAMETER_USER@CERN:CH

    if [ ! $? -eq 0 ]; then
        echo "Could not get credentials, exiting."
        exit -1
    fi

    cd $HOME
    ln -sf afs/.glite
    cp -r afs/.globus .
}

# ================================================================================

function fts_dev_setup_common_ssh_keys
{
    echo
    echo "SSH key operations. This step requires that you store your public SSH keys in your"
    echo "AFS folder:"
    echo
    echo " (<your afs root>/.ssh/id_rsa.pub)"
    echo
    
    fts_dev_setup_wait_for_key
    
    echo "Generating SSH keys. Using the default file ($HOME/.ssh/id_rsa.pub)!"
    echo

    mkdir -p $HOME/.ssh
    ssh-keygen -f $HOME/.ssh/id_rsa
    cat $HOME/afs/.ssh/id_rsa.pub >> $HOME/.ssh/authorized_keys2
    chmod 600 $HOME/.ssh/authorized_keys2
}

# ================================================================================

function fts_dev_setup_common_personal_settings
{
    echo
    echo "Executing personal settings. You should put it to <your AFS root>/bin/setup_vnode.sh"
    echo

    if [ -f $HOME/afs/bin/setup_vnode.sh ]; then
        source  $HOME/afs/bin/setup_vnode.sh
    else
        echo "Cannot find $HOME/afs/bin/setup_vnode.sh. It is not an error, do not forget that this file is a hook for your personalized settings."
    fi
}

# ================================================================================

function fts_dev_setup_common_finalize
{
    local USER=$1
    
    echo
    echo "Executing final steps..."
    echo

    chown -R $USER:$USER /home/$USER

    echo
    echo "Setup finished. Log out and log in with user name $USER from the machine"
    echo "that have private key corresponding to the public key stored in your AFS space. Bye!"
    echo
}
