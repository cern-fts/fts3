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

#include "config/ServerConfig.h"
#include "common/Uri.h"
#include "db/generic/SingleDbInstance.h"
#include "server/DrainMode.h"

#include "FetchStaging.h"
#include "qos-daemon/task/BringOnlineTask.h"
#include "qos-daemon/task/HttpBringOnlineTask.h"
#include "qos-daemon/task/PollTask.h"
#include "qos-daemon/task/HttpPollTask.h"

#include <ctime>

void FetchStaging::fetch()
{
    // VO, user dn, storage, space token
    typedef std::tuple<std::string, std::string, std::string> GroupByType;
    StagingSchedulingInterval = fts3::config::ServerConfig::instance().get<boost::posix_time::time_duration>("StagingSchedulingInterval");

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchStaging starting" << commit;

    // we want to be sure that this won't break our fetching thread
    try
    {
        recoverStartedTasks();
    }
    catch (BaseException& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchStaging " << e.what() << commit;
    }
    catch (...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchStaging Fatal error (unknown origin)" << commit;
    }

    while (!boost::this_thread::interruption_requested()) {
        try {
            boost::this_thread::sleep(StagingSchedulingInterval);

            //if we drain a host, no need to check if url_copy are reporting being alive
            if (fts3::server::DrainMode::instance()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more checking stage-in files for this instance!" << commit;
                continue;
            }

            std::map<GroupByType, std::unique_ptr<StagingContext>> tasks;
            std::vector<StagingOperation> files;

            time_t start = time(0);
            db::DBSingleton::instance().getDBObjectInstance()->getFilesForStaging(files);
            time_t end = time(0);
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"FetchStaging\" "
                                            << "func=\"fetch\" "
                                            << "DBcall=\"getFilesForStaging\" " 
                                            << "time=\"" << end - start << "\""
                                            << commit;

            for (auto it_f = files.begin(); it_f != files.end(); ++it_f)
            {
                std::string storage = Uri::parse(it_f->surl).host;
                GroupByType key(it_f->credId, storage, it_f->spaceToken);
                auto it_t = tasks.find(key);
                if (it_t == tasks.end()) {
                    tasks.insert(std::make_pair(
                        key, StagingContext::createStagingContext(QoSServer::instance(), *it_f))
                    );
                }
                else {
                    it_t->second->add(*it_f);
                }
            }

            for (auto it_t = tasks.begin(); it_t != tasks.end(); ++it_t)
            {
                try
                {
                    if (it_t->second->updateState()) {
                        std::string protocol = it_t->second->getStorageProtocol();
                        if (protocol == "http" || protocol == "https" || protocol == "dav" || protocol == "davs") {
                            threadpool.start(new HttpBringOnlineTask(static_cast<HttpStagingContext&&>(std::move(*(it_t->second)))));
                        } else {
                            threadpool.start(new BringOnlineTask(std::move(*(it_t->second))));
                        }
                    }
                }
                catch(UserError const & ex)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << ex.what() << commit;
                }
                catch(...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
                }
            }

        }
        catch (const std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchStaging " << e.what() << commit;
        }
        catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchStaging interruption requested" << commit;
            break;
        }
        catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchStaging Fatal error (unknown origin)" << commit;
        }
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchStaging exiting" << commit;
}


void FetchStaging::recoverStartedTasks()
{
    std::vector<StagingOperation> startedStagingOps;

    try {
        db::DBSingleton::instance().getDBObjectInstance()->getAlreadyStartedStaging(startedStagingOps);
    }
    catch (UserError const & ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
    }
    catch(...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
    }

    std::map<std::string, std::unique_ptr<StagingContext>> tasks;

    for (auto it_f = startedStagingOps.begin(); it_f != startedStagingOps.end(); ++it_f) {
        auto it_t = tasks.find(it_f->token);
        if (it_t == tasks.end()) {
            tasks.insert(std::make_pair(it_f->token,
                                        StagingContext::createStagingContext(QoSServer::instance(), *it_f))
                                        );
        } else {
            it_t->second->add(*it_f);
        }
    }

    for (auto it_t = tasks.begin(); it_t != tasks.end(); ++it_t) {
        try {
            std::set<std::string> urls = it_t->second->getUrls();
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Recovered with token " << it_t->first << commit;
            for (auto ui = urls.begin(); ui != urls.end(); ++ui) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "\t" << *ui << commit;
            }

            std::string protocol = it_t->second->getStorageProtocol();

            if (protocol == "http" || protocol == "https" || protocol == "dav" || protocol == "davs") {
                threadpool.start(new HttpPollTask(static_cast<HttpStagingContext&&>(std::move(*it_t->second)), it_t->first));
            } else {
                threadpool.start(new PollTask(std::move(*it_t->second), it_t->first));
            }
        }
        catch (UserError const & ex) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
        }
        catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
        }
    }
}
