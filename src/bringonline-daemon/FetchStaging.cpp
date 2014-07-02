/*
 * FetchStaging.cpp
 *
 *  Created on: 2 Jul 2014
 *      Author: simonm
 */

#include "FetchStaging.h"

#include "BringOnlineTask.h"

#include "server/DrainMode.h"

#include "cred/cred-utility.h"

extern bool stopThreads;

void FetchStaging::fetch()
{
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

                    std::vector<StagingTask::context_type> files;
                    db::DBSingleton::instance().getDBObjectInstance()->getFilesForStaging(files);

                    std::vector<StagingTask::context_type>::iterator it;
                    for (it = files.begin(); it != files.end(); ++it)
                        {
                            // make sure it is a srm SE
                            std::string & url = boost::get<BringOnlineTask::url>(*it);
                            if (!isSrmUrl(url)) continue;

                            //get the proxy
                            std::string & dn = boost::get<BringOnlineTask::dn>(*it);
                            std::string & dlg_id = boost::get<BringOnlineTask::dlg_id>(*it);
                            std::string & vo = boost::get<BringOnlineTask::vo>(*it);
                            std::string proxy_file = generateProxy(dn, dlg_id);

                            std::string message;
                            if(!BringOnlineTask::checkValidProxy(proxy_file, message))
                                {
                                    proxy_file = get_proxy_cert(
                                                     dn, // user_dn
                                                     dlg_id, // user_cred
                                                     vo, // vo_name
                                                     "",
                                                     "", // assoc_service
                                                     "", // assoc_service_type
                                                     false,
                                                     ""
                                                 );
                                }

                            try
                                {
                                    threadpool.start(new BringOnlineTask(*it, proxy_file));
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
