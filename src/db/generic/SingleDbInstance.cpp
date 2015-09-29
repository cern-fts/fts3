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

#include "SingleDbInstance.h"
#include <fstream>

#include "../../common/Exceptions.h"
#include "common/Logger.h"
#include "config/serverconfig.h"
#include "version.h"
#include "../profiled/Profiled.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

using namespace fts3::config;
using namespace fts3::common;

namespace db
{


DBSingleton::DBSingleton(): dbBackend(NULL)
{

    std::string dbType = theServerConfig().get<std::string>("DbType");
    std::string versionFTS(VERSION);

    libraryFileName = "libfts_db_";
    libraryFileName += dbType;
    libraryFileName += ".so";
    libraryFileName += ".";
    libraryFileName += versionFTS;

    dlm = new DynamicLibraryManager(libraryFileName);
    if (dlm && dlm->isLibraryLoaded())
        {

            DynamicLibraryManager::Symbol symbolInstatiate = dlm->findSymbol("create");

            DynamicLibraryManager::Symbol symbolDestroy = dlm->findSymbol("destroy");

            *(void**)( &create_db ) =  symbolInstatiate;

            *(void**)( &destroy_db ) = symbolDestroy;

            // create an instance of the DB class
            dbBackend = create_db();

            // If profiling is enabled, wrap it!
            int profileDumpInterval = theServerConfig().get<int>("Profiling");
            if (profileDumpInterval)
                {
                    dbBackend = new ProfiledDB(dbBackend, destroy_db);
                    destroy_db = destroy_profiled_db;
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Database wrapped in the profiler!" << commit;
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Should report every "
                                                    << profileDumpInterval << " seconds" << commit;
                }

            // create monitoring db on request
        }
    else
        {
            if(dlm)
                {
                    throw SystemError(dlm->getLastError());
                }
            else
                {
                    throw SystemError("Can't load " + libraryFileName + " plugin" );
                }
        }
}

DBSingleton::~DBSingleton()
{
    if (dbBackend)
        destroy_db(dbBackend);
    if (dlm)
        delete dlm;
}
}
