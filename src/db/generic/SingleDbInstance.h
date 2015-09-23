/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/**
 * To use the generic database interface one must:
 *  -include "#include "SingleDbInstance.h"
 *  -use namespace db
 *  -call DBSingleton::instance().getDBObjectInstance()->init(username, password, connectString)
 *  -call DBSingleton::instance().getDBObjectInstance()->XXX (API available in GenericDbIfce.h)
 *
 * Notes:
 * -always wrap database calls inside a try/catch block
 * -if an exception is raised, the operation is rollbacked and it's up the user/caller to redo it or not
 **/

#pragma once

#include <iostream>

#include "common/Singleton.h"
#include "GenericDbIfce.h"
#include "DynamicLibraryManager.h"

namespace db
{

/**
 * DBSingleton class declaration
 **/
class DBSingleton: public fts3::common::Singleton<DBSingleton>
{
public:
    DBSingleton();
    ~DBSingleton();

    /**
     * used by the client to obtain access to the backend instance
     **/
    GenericDbIfce* getDBObjectInstance()
    {
        return dbBackend;
    }

private:
    DynamicLibraryManager *dlm;
    std::string libraryFileName;

    /**
     * The types of the database class factories
     **/
    GenericDbIfce* dbBackend;

    GenericDbIfce* (*create_db)();
    void (*destroy_db)(void *);
};

}
