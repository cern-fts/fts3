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

#include "FetchDeletion.h"

#include "../task/DeletionTask.h"
#include "../context/DeletionContext.h"

#include "server/DrainMode.h"
#include "db/generic/SingleDbInstance.h"
#include <map>

#include "../../common/Uri.h"
#include "cred/CredUtility.h"

extern bool stopThreads;


void FetchDeletion::fetch()
{
    try
        {
            db::DBSingleton::instance().getDBObjectInstance()->revertDeletionToStarted();
        }
    catch (BaseException& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION " << e.what() << commit;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION Fatal error (unknown origin)" << commit;
        }

    while (!stopThreads)
        {
            try  //this loop must never exit
                {
                    //if we drain a host, stop with deletions
                    if (fts3::server::DrainMode::instance())
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DELETION Set to drain mode, stopped deleting files with this instance!" << commit;
                            boost::this_thread::sleep(boost::posix_time::milliseconds(15000));
                            continue;
                        }

                    std::map<key_type, DeletionContext> tasks;
                    std::map<key_type, DeletionContext>::iterator it_t;

                    std::vector<DeletionContext::context_type> files;
                    db::DBSingleton::instance().getDBObjectInstance()->getFilesForDeletion(files);

                    std::vector<DeletionContext::context_type>::iterator it_f;
                    for (it_f = files.begin(); it_f != files.end() && !stopThreads; ++it_f)
                        {
                            // make sure it is a srm SE
                            std::string const & url = boost::get<DeletionContext::source_url>(*it_f);
                            // get the SE name
                            Uri uri = Uri::parse(url);
                            std::string se = uri.host;
                            // get the other values necessary for the key
                            std::string const & dn = boost::get<DeletionContext::user_dn>(*it_f);
                            std::string const & vo = boost::get<DeletionContext::vo_name>(*it_f);

                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DELETION To be deleted: \"" << dn << "\"  \"" << vo << "\"  " << url << commit;

                            key_type key(vo, dn, se);
                            it_t = tasks.find(key);
                            if (it_t == tasks.end())
                                tasks.insert(std::make_pair(key, DeletionContext(*it_f)));
                            else
                                it_t->second.add(*it_f);
                        }

                    for (it_t = tasks.begin(); it_t != tasks.end() && !stopThreads; ++it_t)
                        {
                            try
                                {
                                    threadpool.start(new DeletionTask(it_t->second));
                                }
                            catch(UserError const & ex)
                                {
                                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
                                }
                            catch(...)
                                {
                                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
                                }
                        }

                    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                }
            catch (BaseException& e)
                {
                    sleep(2);
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION " << e.what() << commit;
                }
            catch (...)
                {
                    sleep(2);
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION Fatal error (unknown origin)" << commit;
                }
        }
}
