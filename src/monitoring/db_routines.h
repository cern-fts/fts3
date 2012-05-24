#ifndef DB_ROUTINES_H
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
 *  This is singleton class to be used(if needed) for retrieving information from the database
 *  Currently, the following info is retrieved:
 *	-VO
 *      -source site name
 *      -dest site name
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
#include <occi.h>
#include <iostream>
#include <vector>

using namespace std;

class db_routines {
private:
    static bool instanceFlag;
    static db_routines *single;
    static oracle::occi::Environment* env;
    static oracle::occi::Connection* conn;
    static oracle::occi::Statement* stmt;
    static oracle::occi::ResultSet* rs;
    static vector <std::string> credentials;
    static vector<std::string> temp;
    db_routines();
    static std::string errorMessage;

public:
    static db_routines* getInstance();
    static vector<std::string> getData(const char * id);
    ~db_routines();
};

#endif
