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

#include "FetchStaging.h"

#include "../task/BringOnlineTask.h"
#include "../task/PollTask.h"
#include "../task/WaitingRoom.h"

#include "server/DrainMode.h"

#include "db/generic/SingleDbInstance.h"

#include <map>

#include "../../common/Uri.h"
#include "../../cred/CredUtility.h"

extern bool stopThreads;

void FetchStaging::fetch()
{
    WaitingRoom<PollTask>::instance().attach(threadpool);

    try  // we want to be sure that this won't break our fetching thread
        {
            recoverStartedTasks();
        }
    catch (Err& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE " << e.what() << commit;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Fatal error (unknown origin)" << commit;
        }

    while (!stopThreads)
        {
            try  //this loop must never exit
                {
                    //if we drain a host, no need to check if url_copy are reporting being alive
                    if (DrainMode::getInstance())
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more checking stage-in files for this instance!" << commit;
                            boost::this_thread::sleep(boost::posix_time::milliseconds(60000));
                            continue;
                        }

                    std::map<key_type, StagingContext> tasks;
                    std::map<key_type, StagingContext>::iterator it_t;

                    std::vector<StagingContext::context_type> files;
                    db::DBSingleton::instance().getDBObjectInstance()->getFilesForStaging(files);

                    std::vector<StagingContext::context_type>::iterator it_f;
                    for (it_f = files.begin(); it_f != files.end(); ++it_f)
                        {
                            // get the SE name
                            std::string const & url = boost::get<StagingContext::surl>(*it_f);
                            Uri uri = Uri::parse(url);
                            std::string se = uri.host;
                            // get the other values necessary for the key
                            std::string const & dn = boost::get<StagingContext::dn>(*it_f);
                            std::string const & vo = boost::get<StagingContext::vo>(*it_f);
                            std::string const & space_token = boost::get<StagingContext::src_space_token>(*it_f);

                            key_type key(vo, dn, se, space_token);
                            it_t = tasks.find(key);
                            if (it_t == tasks.end())
                                tasks.insert(std::make_pair(key, StagingContext(*it_f)));
                            else
                                it_t->second.add(*it_f);
                        }

                    for (it_t = tasks.begin(); it_t != tasks.end(); ++it_t)
                        {
                            try
                                {
                                    threadpool.start(new BringOnlineTask(it_t->second));
                                }
                            catch(Err_Custom const & ex)
                                {
                                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
                                }
                            catch(...)
                                {
                                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
                                }
                        }

                    boost::this_thread::sleep(boost::posix_time::milliseconds(10000)); //10 secs interval
                }
            catch (Err& e)
                {
                    sleep(2);
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE " << e.what() << commit;
                }
            catch (...)
                {
                    sleep(2);
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Fatal error (unknown origin)" << commit;
                }
        }
}

void FetchStaging::recoverStartedTasks()
{
    std::vector< boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string, std::string> > files;
    std::vector< boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string, std::string> >::const_iterator it_f;

    try
        {
            db::DBSingleton::instance().getDBObjectInstance()->getAlreadyStartedStaging(files);
        }
    catch(Err_Custom const & ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
        }

    std::map<std::string, StagingContext> tasks;
    std::map<std::string, StagingContext>::iterator it_t;

    for (it_f = files.begin(); it_f != files.end(); ++it_f)
        {
            std::string const & token = boost::get<9>(*it_f);
            it_t = tasks.find(token);
            if (it_t == tasks.end())
                tasks.insert(std::make_pair(token, StagingContext(get_context(*it_f))));
            else
                it_t->second.add(get_context(*it_f));
        }

    for (it_t = tasks.begin(); it_t != tasks.end(); ++it_t)
        {
            try
                {
                    threadpool.start(new PollTask(it_t->second, it_t->first));
                }
            catch(Err_Custom const & ex)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
                }
            catch(...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
                }
        }
}
