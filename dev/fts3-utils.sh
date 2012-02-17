#!/bin/bash

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

## Geneneral utilities for FTS 3 development scripts.
## @file

## List of supported platforms.
FTS3_SUPPORTED_PLATFORMS="ubuntu"

## If a function does not support a particulat platform, this is the error message displayed. 
function fts3_wrong_platform_error
{
    echo
    echo "Wrong platform error."
    echo
    echo "   PLATFORM must be set. It must be one of the supported platforms, the string must equal to the " 
    echo "   ETICS build platform. In case of FTS3, the platforms may be:"
    echo
    echo "   $FTS3_SUPPORTED_PLATFORMS"
    echo 
    echo "Your actual value:"
    echo 
    echo "FTS3_PLATFORM=$FTS3_PLATFORM"
    echo

    exit -1
}  

# Check if a andatory variable is defined. It may suggest default value in the error message.
function fts3_utils_check_if_variable_defined
{
    local VAR_NAME=$1
    local VAR_DEFAULT=$2
    local VAR_VALUE=$3

    # color codes, color howto: http://webhome.csc.uvic.ca/~sae/seng265/fall04/tips/s265s047-tips/bash-using-colors.html
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

# CHecks if FTS3_PLATFORM variable is defined. It is a basic, mandatory variable!
function fts3_utils_check_platform
{
    check_if_variable_defined PLATFORM "ubuntu" $FTS3_PLATFORM

    if [ ! $? -eq 0 ]; then
        wrong_platform_error
        exit -1
    fi
}

# Wait for a keypress if there is no argument given. 
function fts3_utils_wait_for_key
{
    local noask=$1

    if [ ! -n "$noask" ]; then
        echo "Press a key if ready to continue."
        read foo
    fi
}


