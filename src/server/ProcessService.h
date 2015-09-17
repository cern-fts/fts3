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
#ifndef PROCESSSERVICE_H_
#define PROCESSSERVICE_H_

#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

#include "db/generic/SingleDbInstance.h"

#include "common/definitions.h"
#include "common/error.h"
#include "common/ThreadPool.h"
#include "common/name_to_uid.h"
#include "common/producer_consumer_common.h"
#include "common/Logger.h"
#include "common/ThreadSafeList.h"
#include "common/Uri.h"

#include "config/serverconfig.h"

#include "cred/CredUtility.h"
#include "cred/DelegCred.h"

#include "profiler/Profiler.h"
#include "profiler/Macros.h"

#include "FileTransferScheduler.h"
#include "FileTransferExecutor.h"
#include "TransferFileHandler.h"
#include "ConfigurationAssigner.h"
#include "ProtocolResolver.h"
#include "DrainMode.h"
#include "ExecuteProcess.h"
#include "ws/SingleTrStateInstance.h"
#include "oauth.h"


extern bool stopThreads;
extern time_t retrieveRecords;

namespace fts3 {
namespace server {


class ProcessService: public boost::noncopyable
{
public:

    /// Constructor
    ProcessService()
    {
        cmd = "fts_url_copy";

        logDir = config::theServerConfig().get<std::string>("TransferLogDirectory");
        execPoolSize = config::theServerConfig().get<int>("InternalThreadPool");
        ftsHostName = config::theServerConfig().get<std::string>("Alias");
        infosys = config::theServerConfig().get<std::string>("Infosys");
        const std::vector<std::string> voNameList(
                config::theServerConfig().get<std::vector<std::string> >("AuthorizedVO"));

        if (voNameList.size() > 0 && voNameList[0].compare("*") != 0)
        {
            allowedVOs += "(";
            for (auto iterVO = voNameList.begin(); iterVO != voNameList.end(); ++iterVO)
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

        std::string monitoringMessagesStr = config::theServerConfig().get<
                std::string>("MonitoringMessaging");
        if (monitoringMessagesStr == "false")
            monitoringMessages = false;
        else
            monitoringMessages = true;
    }

    /// Destructor
    ~ProcessService()
    {
    }

    void operator () ()
    {
        static bool drainMode = false;

        while (!stopThreads)
        {
            retrieveRecords = time(0);

            try
            {
                if (DrainMode::getInstance())
                {
                    if (!drainMode)
                        FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Set to drain mode, no more transfers for this instance!" << commit;
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
                FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Exception in process_service_handler " << e.what() << commit;
                sleep(2);
            }
            catch (...)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler!" << commit;
                sleep(2);
            }
            sleep(2);
        }
    }

protected:
    std::string ftsHostName;
    std::string allowedVOs;
    std::string infosys;
    bool monitoringMessages;
    int execPoolSize;
    std::string cmd;
    std::string logDir;


    void getFiles(const std::vector<QueueId>& queues)
    {
        try
        {
            if (queues.empty())
                return;

            //now get files to be scheduled
            std::map<std::string, std::list<TransferFile> > voQueues;
            DBSingleton::instance().getDBObjectInstance()->getReadyTransfers(
                    queues, voQueues);

            if (voQueues.empty())
                return;

            // create transfer-file handler
            TransferFileHandler tfh(voQueues);

            // the worker thread pool
            common::ThreadPool<FileTransferExecutor> execPool(execPoolSize);

            std::map<std::pair<std::string, std::string>, std::string> proxies;

            // loop until all files have been served
            int initial_size = tfh.size();

            while (!tfh.empty())
            {
                PROFILE_SCOPE("executeUrlcopy::while[!reuse]");

                // iterate over all VOs
                for (auto it_vo = tfh.begin(); it_vo != tfh.end(); it_vo++)
                {
                    if (stopThreads)
                    {
                        execPool.interrupt();
                        return;
                    }

                    boost::optional<TransferFile> opt_tf = tfh.get(*it_vo);
                    // if this VO has no more files to process just continue
                    if (!opt_tf)
                        continue;

                    TransferFile & tf = *opt_tf;

                    // just to be sure
                    if (tf.fileId == 0 || tf.userDn.empty()
                            || tf.credId.empty())
                        continue;

                    std::pair<std::string, std::string> proxy_key(tf.credId,
                            tf.userDn);

                    if (proxies.find(proxy_key) == proxies.end())
                    {
                        proxies[proxy_key] = DelegCred::getProxyFile(tf.userDn,
                                tf.credId);
                    }

                    FileTransferExecutor* exec = new FileTransferExecutor(tf,
                            tfh, monitoringMessages, infosys, ftsHostName,
                            proxies[proxy_key], logDir);

                    execPool.start(exec);

                }
            }

            // wait for all the workers to finish
            execPool.join();
            FTS3_COMMON_LOGGER_NEWLOG(INFO) <<"Threadpool processed: " << initial_size
                    << " files (" << execPool.reduce(std::plus<int>())
                    << " have been scheduled)" << commit;

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
        std::vector<QueueId> queues;

        try
        {
            boost::thread_group g;

            try
            {
                DBSingleton::instance().getDBObjectInstance()->getQueuesWithPending(queues);
            }
            catch (std::exception& e)
            {
                //try again if deadlocked
                sleep(1);
                try
                {
                    queues.clear();
                    DBSingleton::instance().getDBObjectInstance()->getQueuesWithPending(queues);
                }
                catch (std::exception& e)
                {
                    queues.clear();

                }
                catch (...)
                {
                    queues.clear();
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler!" << commit;
                }
            }
            catch (...)
            {
                //try again if deadlocked
                sleep(1);
                try
                {
                    queues.clear();
                    DBSingleton::instance().getDBObjectInstance()->getQueuesWithPending(queues);
                }
                catch (std::exception& e)
                {
                    queues.clear();
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler " << e.what() << commit;
                }
                catch (...)
                {
                    queues.clear();
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler!" << commit;
                }
            }

            if(queues.empty())
            {
                return;
            }
            else if(1 == queues.size())
            {
                getFiles(queues);
            }
            else
            {
                std::size_t const half_size1 = queues.size() / 2;
                std::vector<QueueId> split_1(queues.begin(), queues.begin() + half_size1);
                std::vector<QueueId> split_2(queues.begin() + half_size1, queues.end());

                std::size_t const half_size2 = split_1.size() / 2;
                std::vector<QueueId> split_11(split_1.begin(), split_1.begin() + half_size2);
                std::vector<QueueId> split_21(split_1.begin() + half_size2, split_1.end());

                std::size_t const half_size3 = split_2.size() / 2;
                std::vector<QueueId> split_12(split_2.begin(), split_2.begin() + half_size3);
                std::vector<QueueId> split_22(split_2.begin() + half_size3, split_2.end());

                //create threads only when needed
                if(!split_11.empty())
                g.create_thread(boost::bind(&ProcessService::getFiles, this, boost::ref(split_11)));
                if(!split_21.empty())
                g.create_thread(boost::bind(&ProcessService::getFiles, this, boost::ref(split_21)));
                if(!split_12.empty())
                g.create_thread(boost::bind(&ProcessService::getFiles, this, boost::ref(split_12)));
                if(!split_22.empty())
                g.create_thread(boost::bind(&ProcessService::getFiles, this, boost::ref(split_22)));

                // wait for them
                g.join_all();
            }

            queues.clear();
        }
        catch (std::exception& e)
        {
            queues.clear();
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler " << e.what() << commit;
        }
        catch (...)
        {
            queues.clear();
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler!" << commit;
        }
    }
};

} // end namespace server
} // end namespace fts3

#endif // PROCESSSERVICE_H_
