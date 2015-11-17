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

#include <map>

#include "common/Uri.h"
#include "cred/CredUtility.h"
#include "db/generic/SingleDbInstance.h"
#include "server/DrainMode.h"

#include "../task/DeletionTask.h"
#include "../context/DeletionContext.h"

#include "FetchDeletion.h"



/// At the beginning of the subsystem, revert
/// started delete operations assigned to this host
// back to started, so they re-enter the queue
static void revertDeleteToStart()
{
    try {
        db::DBSingleton::instance().getDBObjectInstance()->requeueStartedDeletes();
    }
    catch (BaseException& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION " << e.what() << commit;
    }
    catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION Fatal error (unknown origin)" << commit;
    }
}


void FetchDeletion::fetch()
{
    typedef std::tuple<std::string, std::string, std::string> Triplet;

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchDeletion starting" << commit;

    revertDeleteToStart();

    while (!boost::this_thread::interruption_requested())
    {
        try {
            //if we drain a host, stop with deletions
            if (fts3::server::DrainMode::instance())
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchDeletion Set to drain mode, stopped deleting files with this instance!" << commit;
                boost::this_thread::sleep(boost::posix_time::milliseconds(15000));
                continue;
            }

            boost::this_thread::sleep(boost::posix_time::seconds(10));

            std::map<Triplet, DeletionContext> deletionContext;
            std::vector<DeleteOperation> deletions;

            db::DBSingleton::instance().getDBObjectInstance()->getFilesForDeletion(deletions);

            // Generate a list of deletion contexts, grouping urls belonging to the same
            // storage under the same context
            for (auto i = deletions.begin(); i != deletions.end() && !boost::this_thread::interruption_requested(); ++i) {
                const std::string storage = Uri::parse(i->surl).host;

                FTS3_COMMON_LOGGER_NEWLOG(INFO)
                    << "DELETION To be deleted: \""
                    << i->userDn << "\"  \"" << i->voName << "\"  " << i->surl
                    << commit;

                Triplet key(i->voName, i->userDn, storage);
                auto itContext = deletionContext.find(key);
                if (itContext == deletionContext.end()) {
                    deletionContext.insert(std::make_pair(key, DeletionContext(*i)));
                }
                else {
                    itContext->second.add(*i);
                }
            }

            // Trigger a task per populated context
            for (auto i = deletionContext.begin(); i != deletionContext.end() && !boost::this_thread::interruption_requested(); ++i) {
                try {
                    threadpool.start(new DeletionTask(i->second));
                }
                catch(UserError const & ex) {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
                }
                catch(...) {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
                }
            }
        }
        catch (const std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchDeletion " << e.what() << commit;
        }
        catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchDeletion interruption requested" << commit;
            break;
        }
        catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchDeletion Fatal error (unknown origin)" << commit;
        }
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchDeletion exiting" << commit;
}
