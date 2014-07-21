/*
 * FetchStaging.cpp
 *
 *  Created on: 2 Jul 2014
 *      Author: simonm
 */

#include "FetchStaging.h"

#include "BringOnlineTask.h"
#include "PollTask.h"
#include "WaitingRoom.h"

#include "server/DrainMode.h"

#include "cred/cred-utility.h"

#include "db/generic/SingleDbInstance.h"

#include "common/parse_url.h"

#include <map>

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
                            boost::this_thread::sleep(boost::posix_time::milliseconds(15000));
                            continue;
                        }

                    std::map<key_type, StagingContext> tasks;

                    std::vector<StagingContext::context_type> files;
                    db::DBSingleton::instance().getDBObjectInstance()->getFilesForStaging(files);

                    std::vector<StagingContext::context_type>::iterator it_f;
                    for (it_f = files.begin(); it_f != files.end(); ++it_f)
                        {
                            // make sure it is a srm SE
                            std::string & url = boost::get<StagingContext::surl>(*it_f);
                            if (!isSrmUrl(url)) continue;
                            // get the SE name
                            Uri uri = Uri::Parse(boost::get<StagingContext::surl>(*it_f));
                            std::string se = uri.Host;
                            // get the other values necessary for the key
                            std::string & dn = boost::get<StagingContext::dn>(*it_f);
                            std::string & vo = boost::get<StagingContext::vo>(*it_f);
                            std::string& space_token = boost::get<StagingContext::src_space_token>(*it_f);

                            tasks[key_type(vo, dn, se, space_token)].add(*it_f);
                        }

                    std::map<key_type, StagingContext>::const_iterator it_t;
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

                    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                }
            catch (Err& e)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE " << e.what() << commit;
                }
            catch (...)
                {
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

    for (it_f = files.begin(); it_f != files.end(); ++it_f)
        {
            std::string const & token = boost::get<9>(*it_f);
            tasks[token].add(get_context(*it_f));
        }

    std::map<std::string, StagingContext>::const_iterator it_t;

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
