#ifndef BDII_ROUTINES_H
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

#include <iostream>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <log4cpp/Category.hh>

#include "ServiceDiscoveryIfce.h"

using namespace std;


class bdii_routines {
private:
    static bool instanceFlag;
    static bdii_routines *single;

    bdii_routines() //private constructor
    {
    }


public:
    static bdii_routines* getInstance();
    std::string getEndpoint(std::string & value, log4cpp::CategoryStream error);

    ~bdii_routines();
};

#endif
