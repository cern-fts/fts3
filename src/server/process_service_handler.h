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

#pragma once

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <string>
#include "SingleDbInstance.h"
#include "common/logger.h"
#include "common/error.h"
#include "common/ThreadPool.h"
#include "process.h"
#include <iostream>
#include <map>
#include <list>
#include <string>
#include <vector>
#include <sstream>
#include "site_name.h"
#include "FileTransferScheduler.h"
#include "FileTransferExecutor.h"
#include "TransferFileHandler.h"
#include "ConfigurationAssigner.h"
#include "ProtocolResolver.h"
#include "DelegCred.h"
#include <signal.h>
#include "parse_url.h"
#include "cred-utility.h"
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fstream>
#include "config/serverconfig.h"
#include "definitions.h"
#include "DrainMode.h"
#include "queue_updater.h"
#include <boost/algorithm/string.hpp>
#include <sys/param.h>
#include "name_to_uid.h"
#include "producer_consumer_common.h"
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <boost/algorithm/string/replace.hpp>
#include "ws/SingleTrStateInstance.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "profiler/Profiler.h"
#include "profiler/Macros.h"
#include <boost/thread.hpp>
#include "oauth.h"

extern bool stopThreads;
extern time_t retrieveRecords;

namespace fts3 {
namespace server {

template
<
typename TRAITS
>
class ProcessServiceHandler : public TRAITS::ActiveObjectType
{
protected:

    using TRAITS::ActiveObjectType::_enqueue;

public:

    /* ---------------------------------------------------------------------- */

//    typedef ProcessServiceHandler <TRAITS> OwnType;

    /* ---------------------------------------------------------------------- */

    /** Constructor. */
    ProcessServiceHandler
    (
        const std::string& desc = "" /**< Description of this service handler
            (goes to log) */
    ) :
        TRAITS::ActiveObjectType("ProcessServiceHandler", desc)
    {
        cmd = "fts_url_copy";

	logDir = config::theServerConfig().get<std::string > ("TransferLogDirectory");
        execPoolSize = config::theServerConfig().get<int> ("InternalThreadPool");
        ftsHostName = config::theServerConfig().get<std::string > ("Alias");
        allowedVOs = std::string("");
        infosys = config::theServerConfig().get<std::string > ("Infosys");
        const std::vector<std::string> voNameList(config::theServerConfig().get< std::vector<std::string> >("AuthorizedVO"));
        if (voNameList.size() > 0 && std::string(voNameList[0]).compare("*") != 0)
            {
                std::vector<std::string>::const_iterator iterVO;
                allowedVOs += "(";
                for (iterVO = voNameList.begin(); iterVO != voNameList.end(); ++iterVO)
                    {
                        allowedVOs += "'";
                        allowedVOs += (*iterVO);
                        allowedVOs += "',";
                    }
                allowedVOs = allowedVOs.substr(0, allowedVOs.size() - 1);
                allowedVOs += ")";
                boost::algorithm::to_lower(allowedVOs);
            }
        else
            {
                allowedVOs = voNameList[0];
            }

        std::string monitoringMessagesStr = config::theServerConfig().get<std::string > ("MonitoringMessaging");
        if(monitoringMessagesStr == "false")
            monitoringMessages = false;
        else
            monitoringMessages = true;

    }

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ProcessServiceHandler()
    {
    }

    /* ---------------------------------------------------------------------- */

    void executeTransfer_p
    (
    )
    {

        boost::function<void() > op = boost::bind(&ProcessServiceHandler::executeTransfer_a, this);
        this->_enqueue(op);
    }

protected:
    SiteName siteResolver;
    std::string ftsHostName;
    std::string allowedVOs;
    std::string infosys;
    bool monitoringMessages;
    int execPoolSize;
    std::string cmd;
    std::string logDir;

    std::string extractHostname(const std::string &surl)
    {
        Uri u0 = Uri::Parse(surl);
        return u0.Protocol + "://" + u0.Host;
    }

    void getFiles( std::vector< boost::tuple<std::string, std::string, std::string> >& distinct)
    {
        try
            {
                if(distinct.empty())
                    return;

                //now get files to be scheduled
                std::map< std::string, std::list<TransferFiles> > voQueues;
                DBSingleton::instance().getDBObjectInstance()->getByJobId(distinct, voQueues);

                if(voQueues.empty())
                    return;

                // create transfer-file handler
                TransferFileHandler tfh(voQueues);

                // the worker thread pool
                common::ThreadPool<FileTransferExecutor> execPool(execPoolSize);

                std::map< std::pair<std::string, std::string>, std::string > proxies;

                // loop until all files have been served

                int initial_size = tfh.size();


                while (!tfh.empty())
                    {
                        PROFILE_SCOPE("executeUrlcopy::while[!reuse]");

                        // iterate over all VOs
                        set<string>::iterator it_vo;
                        for (it_vo = tfh.begin(); it_vo != tfh.end(); it_vo++)
                            {
                                if (stopThreads)
                                    {
                                        execPool.interrupt();
                                        return;
                                    }

                                boost::optional<TransferFiles> opt_tf = tfh.get(*it_vo);
                                // if this VO has no more files to process just continue
                                if (!opt_tf) continue;

                                TransferFiles & tf = *opt_tf;

                                // just to be sure
                                if(tf.FILE_ID == 0 || tf.DN.empty() || tf.CRED_ID.empty()) continue;

                                std::pair<std::string, std::string> proxy_key(tf.CRED_ID, tf.DN);

                                if (proxies.find(proxy_key) == proxies.end())
                                    {
                                        std::unique_ptr<DelegCred> delegCredPtr(new DelegCred);
                                        std::string filename = delegCredPtr->getFileName(tf.DN, tf.CRED_ID), message;

                                        if (!delegCredPtr->isValidProxy(filename, message))
                                            {
                                                if(!message.empty())
                                                    {
                                                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << message  << commit;
                                                    }
                                                // check the proxy lifetime in DB
                                                time_t db_lifetime = -1;
                                                std::unique_ptr<Cred> cred (DBSingleton::instance().getDBObjectInstance()->
                                                                              findCredential(tf.CRED_ID, tf.DN)
                                                                             );
                                                if (cred.get()) db_lifetime = cred->termination_time - time(NULL);
                                                // check the proxy lifetime in filesystem
                                                time_t lifetime, voms_lifetime;
                                                get_proxy_lifetime(filename, &lifetime, &voms_lifetime);

                                                if (db_lifetime > lifetime)
                                                    {
                                                        filename = get_proxy_cert(
                                                                       tf.DN, // user_dn
                                                                       tf.CRED_ID, // user_cred
                                                                       tf.VO_NAME, // vo_name
                                                                       "",
                                                                       "", // assoc_service
                                                                       "", // assoc_service_type
                                                                       false,
                                                                       ""
                                                                   );
                                                    }
                                            }

                                        proxies[proxy_key] = filename;
                                    }

                                FileTransferExecutor* exec = new FileTransferExecutor(
                                    tf,
                                    tfh,
                                    monitoringMessages,
                                    infosys,
                                    ftsHostName,
                                    proxies[proxy_key],
				    logDir
                                );

                                execPool.start(exec);

                            }
                    }

                // wait for all the workers to finish
                execPool.join();
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Threadpool processed: " << initial_size << " files (" << execPool.reduce(std::plus<int>()) << " have been scheduled)" << commit;

            }
        catch (std::exception& e)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler:getFiles " << e.what() << commit;
            }
        catch (...)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler!" << commit;
            }
    }

    void executeUrlcopy()
    {
        //get distinct source, dest, vo first
        std::vector< boost::tuple<std::string, std::string, std::string> > distinct;

        try
            {
                boost::thread_group g;

                try
                    {
                        DBSingleton::instance().getDBObjectInstance()->getVOPairs(distinct);
                    }
                catch (std::exception& e)
                    {
                        //try again if deadlocked
                        sleep(1);
                        try
                            {
                                distinct.clear();
                                DBSingleton::instance().getDBObjectInstance()->getVOPairs(distinct);
                            }
                        catch (std::exception& e)
                            {
                                distinct.clear();
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler " << e.what() << commit;
                            }
                        catch (...)
                            {
                                distinct.clear();
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler!" << commit;
                            }
                    }
                catch (...)
                    {
                        //try again if deadlocked
                        sleep(1);
                        try
                            {
                                distinct.clear();
                                DBSingleton::instance().getDBObjectInstance()->getVOPairs(distinct);
                            }
                        catch (std::exception& e)
                            {
                                distinct.clear();
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler " << e.what() << commit;
                            }
                        catch (...)
                            {
                                distinct.clear();
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler!" << commit;
                            }
                    }

                if(distinct.empty())
                    {
                        return;
                    }
                else if(1 == distinct.size())
                    {
                        getFiles(distinct);
                    }
                else
                    {
                        std::size_t const half_size1 = distinct.size() / 2;
                        std::vector< boost::tuple<std::string, std::string, std::string> > split_1(distinct.begin(), distinct.begin() + half_size1);
                        std::vector< boost::tuple<std::string, std::string, std::string> > split_2(distinct.begin() + half_size1, distinct.end());

                        std::size_t const half_size2 = split_1.size() / 2;
                        std::vector< boost::tuple<std::string, std::string, std::string> > split_11(split_1.begin(), split_1.begin() + half_size2);
                        std::vector< boost::tuple<std::string, std::string, std::string> > split_21(split_1.begin() + half_size2, split_1.end());

                        std::size_t const half_size3 = split_2.size() / 2;
                        std::vector< boost::tuple<std::string, std::string, std::string> > split_12(split_2.begin(), split_2.begin() + half_size3);
                        std::vector< boost::tuple<std::string, std::string, std::string> > split_22(split_2.begin() + half_size3, split_2.end());


                        //create threads only when needed
                        if(!split_11.empty())
                            g.create_thread(boost::bind(&ProcessServiceHandler::getFiles, this, boost::ref(split_11)));
                        if(!split_21.empty())
                            g.create_thread(boost::bind(&ProcessServiceHandler::getFiles, this, boost::ref(split_21)));
                        if(!split_12.empty())
                            g.create_thread(boost::bind(&ProcessServiceHandler::getFiles, this, boost::ref(split_12)));
                        if(!split_22.empty())
                            g.create_thread(boost::bind(&ProcessServiceHandler::getFiles, this, boost::ref(split_22)));

                        // wait for them
                        g.join_all();
                    }

                distinct.clear();
            }
        catch (std::exception& e)
            {
                distinct.clear();
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler " << e.what() << commit;
            }
        catch (...)
            {
                distinct.clear();
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler!" << commit;
            }
    }

    /* ---------------------------------------------------------------------- */

    void executeTransfer_a()
    {
        static bool drainMode = false;

        while (true)
            {
                retrieveRecords = time(0);

                try
                    {
                        if (stopThreads)
                            {
                                return;
                            }

                        if (DrainMode::getInstance())
                            {
                                if (!drainMode)
                                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more transfers for this instance!" << commit;
                                drainMode = true;
                                sleep(15);
                                continue;
                            }
                        else
                            {
                                drainMode = false;
                            }

                        /*check for non-reused jobs*/
                        executeUrlcopy();

                        if (stopThreads)
                            return;
                    }
                catch (std::exception& e)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler " << e.what() << commit;
                        sleep(2);
                    }
                catch (...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler!" << commit;
                        sleep(2);
                    }
                sleep(2);
            } /*end while*/
    }

    /* ---------------------------------------------------------------------- */
    struct TestHelper
    {

        TestHelper()
            : loopOver(false)
        {
        }

        bool loopOver;
    }
    _testHelper;
};

} // end namespace server
} // end namespace fts3

