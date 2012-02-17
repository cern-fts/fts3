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


#pragma once

#include <pthread.h>
#include <iostream>
#include <boost/scoped_ptr.hpp>
#include "MutexLocker.h"
#include "GenericDbIfce.h"
#include "DynamicLibraryManager.h"


/**
 * DBSingleton class declaration
 **/ 
class DBSingleton {
public:
    ~DBSingleton();

/**
 * DBSingleton thread-safe upon init singleton
 **/ 
    static DBSingleton & instance() {
        if (i.get() == 0) {
            MutexLocker obtain_lock(m);
            if (i.get() == 0) {
                i.reset(new DBSingleton);
            }
        }
        return *i;
    }

/**
 * used by the client to obtain access to the backend instance
 **/ 
    GenericDbIfce* getDBObjectInstance() {
        return dbBackend;
    }


private:
    DBSingleton(); // Private so that it can  not be called

    DBSingleton(DBSingleton const&) {
    }; // copy constructor is private

    DBSingleton & operator=(DBSingleton const&) {
    }; // assignment operator is private
    static boost::scoped_ptr<DBSingleton> i;
    static Mutex m;

    GenericDbIfce* dbBackend;

    create_t* create_db;
    destroy_t* destroy_db;
    DynamicLibraryManager *dlm;
    std::string libraryFileName;
};


