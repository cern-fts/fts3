/**
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://www.eu-egee.org/partners/ for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This is singleton class to be used(if needed) for retrieving information from the BDII
 */

#include "bdii_routines.h"

bool bdii_routines::instanceFlag = false;
bdii_routines* bdii_routines::single = NULL;

bdii_routines* bdii_routines::getInstance() {
    if (!instanceFlag) {
        single = new bdii_routines();
        instanceFlag = true;
        return single;
    } else {
        return single;
    }
}

bdii_routines::~bdii_routines() {

    instanceFlag = false;
}


std::string bdii_routines::getEndpoint(std::string & value, log4cpp::CategoryStream error) {
   SDException exc;
    value = "*" + value + "*";
    char *dest_site_name = SD_getServiceSite(value.c_str(), &exc);
    if (dest_site_name == NULL)
        return "";

    return dest_site_name;
}

