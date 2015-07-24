/********************************************//**
 * Copyright @ Members of the EMI Collaboration, 2010.
 * See www.eu-emi.eu for details on the copyright holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***********************************************/

/**
 * @file SingleDbInstance.h
 * @brief single database instance to use for accessing generic db interface
 * @author Michail Salichos
 * @date 09/02/2012
 *
 **/


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
#include <boost/scoped_ptr.hpp>
#include "common/monitorobject.h"
#include "GenericDbIfce.h"
#include "DynamicLibraryManager.h"

using namespace FTS3_COMMON_NAMESPACE;
namespace db
{
/**
 * DBSingleton class declaration
 **/
class DBSingleton: public MonitorObject
{
public:
    ~DBSingleton();

    /**
     * DBSingleton thread-safe upon init singleton
     **/
    static DBSingleton & instance()
    {
        if (i.get() == 0)
            {
                ThreadTraits::LOCK lock(_mutex);
                if (i.get() == 0)
                    {
                        i.reset(new DBSingleton);
                    }
            }
        return *i;
    }

    /**
     * used by the client to obtain access to the backend instance
     **/
    GenericDbIfce* getDBObjectInstance()
    {
        return dbBackend;
    }

    /**
     * Force the destruction of the backend
     * Needed to keep a consistent state on fork!
     */
    static void tearDown()
    {
        if (i.get() != 0)
            {
                i.reset(0);
            }
    }


private:
    DBSingleton(); // Private so that it can  not be called

    DBSingleton(DBSingleton const&)
    {
    }; // copy constructor is private
    static ThreadTraits::MUTEX _mutex;
    DBSingleton & operator=(DBSingleton const&);
    // assignment operator is private
    static boost::scoped_ptr<DBSingleton> i;
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
