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

function fts3_dev_setup_install_tools
{
    echo "Installing the required software on platform $PLATFORM..."
    echo
    
    yum -y --enablerepo slc6-cernonly install \
        doxygen \
        cmake \
        gsoap-devel \
        oracle-instantclient-devel \
        oracle-instantclient-sqlplus \
        oracle-instantclient-basic \
        libuuid-devel
}

