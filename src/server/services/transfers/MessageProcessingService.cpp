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

#include "MessageProcessingService.h"

#include <glib.h>
#include <boost/filesystem.hpp>

#include "common/Exceptions.h"
#include "config/ServerConfig.h"
#include "common/Logger.h"
#include "db/generic/SingleDbInstance.h"
#include "SingleTrStateInstance.h"
#include "ThreadSafeList.h"


using namespace fts3::common;
using fts3::config::ServerConfig;


namespace fts3 {
namespace server {

extern time_t updateRecords;


MessageProcessingService::MessageProcessingService(): BaseService("MessageProcessingService"),
    consumer(ServerConfig::instance().get<std::string>("MessagingDirectory")),
    producer(ServerConfig::instance().get<std::string>("MessagingDirectory"))
{
    enableOptimization = config::ServerConfig::instance().get<std::string > ("Optimizer");
    messages.reserve(600);
}


MessageProcessingService::~MessageProcessingService()
{
}


void MessageProcessingService::runService()
{
    namespace fs = boost::filesystem;

    while (!boost::this_thread::interruption_requested())
    {
        updateRecords = time(0);

        try
        {
            if (boost::this_thread::interruption_requested() && messages.empty() && messagesLog.empty())
            {
                break;
            }

            //if conn to the db is lost, do not retrieve state, save it for later
            //use one fast query
            try
            {
                db::DBSingleton::instance().getDBObjectInstance()->getDrain();
            }
            catch (...) {
                boost::this_thread::sleep(boost::posix_time::seconds(10));
                continue;
            }

            // update statuses
            if (consumer.runConsumerStatus(messages) != 0)
            {
                char buffer[128] = {0};
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not get the status messages:" << strerror_r(errno, buffer, sizeof(buffer)) << commit;
                continue;
            }

            if (!messages.empty())
            {
                executeUpdate(messages);
                db::DBSingleton::instance().getDBObjectInstance()->updateProtocol(messages);
                messages.clear();
            }

            // update log file path
            if (consumer.runConsumerLog(messagesLog) != 0)
            {
                char buffer[128] = { 0 };
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not get the log messages:" << strerror_r(errno, buffer, sizeof(buffer)) << commit;
                continue;
            }

            if (!messagesLog.empty()) {
                db::DBSingleton::instance().getDBObjectInstance()->transferLogFileVector(messagesLog);
                messagesLog.clear();
            }

            // update heartbeat and progress vector
            if (consumer.runConsumerStall(messagesUpdater) != 0)
            {
                char buffer[128] = { 0 };
                FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Could not get the updater messages:" << strerror_r(errno, buffer, sizeof(buffer)) << commit;
                continue;
            }

            if(!messagesUpdater.empty())
            {
                std::vector<fts3::events::MessageUpdater>::iterator iterUpdater;
                for (iterUpdater = messagesUpdater.begin(); iterUpdater != messagesUpdater.end(); ++iterUpdater)
                {
                    std::string job = std::string((*iterUpdater).job_id()).substr(0, 36);
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Process Updater Monitor "
                        << "\nJob id: " << job
                        << "\nFile id: " << (*iterUpdater).file_id()
                        << "\nPid: " << (*iterUpdater).process_id()
                        << "\nTimestamp: " << (*iterUpdater).timestamp()
                        << "\nThroughput: " << (*iterUpdater).throughput()
                        << "\nTransferred: " << (*iterUpdater).transferred()
                        << commit;
                    ThreadSafeList::get_instance().updateMsg(*iterUpdater);
                }

                db::DBSingleton::instance().getDBObjectInstance()->updateFileTransferProgressVector(messagesUpdater);
                messagesUpdater.clear();
            }
        }
        catch (const std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;

            for (auto iterBreak = messages.begin(); iterBreak != messages.end(); ++iterBreak)
            {
                producer.runProducerStatus(*iterBreak);
            }

            std::map<int, fts3::events::MessageLog>::const_iterator iterLogBreak;
            for (iterLogBreak = messagesLog.begin(); iterLogBreak != messagesLog.end(); ++iterLogBreak)
            {
                fts3::events::MessageLog msgLogBreak = (*iterLogBreak).second;
                producer.runProducerLog( msgLogBreak );
            }
        }
        catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue thrown unhandled exception" << commit;

            for (auto iterBreak = messages.begin(); iterBreak != messages.end(); ++iterBreak)
            {
                producer.runProducerStatus(*iterBreak);
            }

            std::map<int, fts3::events::MessageLog>::const_iterator iterLogBreak;
            for (iterLogBreak = messagesLog.begin(); iterLogBreak != messagesLog.end(); ++iterLogBreak)
            {
                fts3::events::MessageLog msgLogBreak = (*iterLogBreak).second;
                producer.runProducerLog( msgLogBreak );
            }
        }
        boost::this_thread::sleep(boost::posix_time::seconds(1));
    }
}


void MessageProcessingService::updateDatabase(const fts3::events::Message& msg)
{
    try
    {
        //do not process the updates here, will be done separately
        if(std::string(msg.transfer_status()).compare("UPDATE") == 0)
            return;

        if (std::string(msg.transfer_status()).compare("FINISHED") == 0 ||
                std::string(msg.transfer_status()).compare("FAILED") == 0 ||
                std::string(msg.transfer_status()).compare("CANCELED") == 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO)
                << "Removing job from monitoring list " << msg.job_id() << " " << msg.file_id()
                << commit;
            ThreadSafeList::get_instance().removeFinishedTr(msg.job_id(), msg.file_id());
        }

        if(std::string(msg.transfer_status()).compare("FAILED") == 0)
        {
            try
            {
                //multiple replica files belonging to a job will not be retried
                int retry = db::DBSingleton::instance().getDBObjectInstance()->getRetry(msg.job_id());

                if(msg.retry() == true && retry > 0 && msg.file_id() > 0)
                {
                    int retryTimes = db::DBSingleton::instance().getDBObjectInstance()->getRetryTimes(msg.job_id(),
                        msg.file_id());
                    if (retryTimes <= retry - 1)
                    {
                        db::DBSingleton::instance().getDBObjectInstance()->setRetryTransfer(
                            msg.job_id(), msg.file_id(), retryTimes+1, msg.transfer_message(), msg.errcode());
                        return;
                    }
                }
            }
            catch (std::exception& e)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception when set retry " << e.what() << commit;
            }
            catch (...)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception when set retry " << commit;
            }
        }

        /*session reuse process died or terminated unexpected, must terminate all files of a given job*/
        if (msg.transfer_message().find("Transfer terminate handler called") != std::string::npos ||
            msg.transfer_message().find("Transfer process died") != std::string::npos ||
            msg.transfer_message().find("because it was stalled") != std::string::npos ||
            msg.transfer_message().find("canceled by the user") != std::string::npos ||
            msg.transfer_message().find("undefined symbol") != std::string::npos ||
            msg.transfer_message().find("canceled because it was not responding") != std::string::npos)
        {
            if(std::string(msg.job_id()).length() == 0)
            {
                db::DBSingleton::instance().getDBObjectInstance()->terminateReuseProcess(
                    std::string(), msg.process_id(), msg.transfer_message());
            }
            else
            {
                db::DBSingleton::instance().getDBObjectInstance()->terminateReuseProcess(
                    msg.job_id(), msg.process_id(), msg.transfer_message());
            }
        }

        //update file/job state
        bool updated = db::DBSingleton::instance().getDBObjectInstance()->updateTransferStatus(
            msg.job_id(), msg.file_id(), msg.throughput(), msg.transfer_status(),
            msg.transfer_message(), msg.process_id(), msg.filesize(), msg.time_in_secs(), msg.retry());

        db::DBSingleton::instance().getDBObjectInstance()->updateJobStatus(
            msg.job_id(), msg.transfer_status(), msg.process_id());

        if (!updated && msg.transfer_status() != "CANCELED") {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Entry in the database not updated for "
                << msg.job_id() << " " << msg.file_id()
                << ". Probably already in a different terminal state"
                << commit;
        }
        else if (!msg.job_id().empty() && msg.file_id() > 0) {
            SingleTrStateInstance::instance().sendStateMessage(msg.job_id(), msg.file_id());
        }
    }
    catch (std::exception& e)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception " << e.what() << commit;
        producer.runProducerStatus(msg);
    }
    catch (...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception" << commit;
        producer.runProducerStatus(msg);
    }
}


void MessageProcessingService::executeUpdate(const std::vector<fts3::events::Message>& messages)
{
    fts3::events::MessageUpdater msgUpdater;

    for (auto iter = messages.begin(); iter != messages.end(); ++iter)
    {
        try
        {
            if(boost::this_thread::interruption_requested())
            {
                for (auto iterBreak = messages.begin(); iterBreak != messages.end(); ++iterBreak)
                {
                    producer.runProducerStatus(*iterBreak);
                }

                std::map<int, fts3::events::MessageLog>::const_iterator iterLogBreak;
                for (iterLogBreak = messagesLog.begin(); iterLogBreak != messagesLog.end(); ++iterLogBreak)
                {
                    fts3::events::MessageLog msgLogBreak = (*iterLogBreak).second;
                    producer.runProducerLog( msgLogBreak );
                }

                break;
            }

            msgUpdater.set_job_id((*iter).job_id());
            msgUpdater.set_file_id((*iter).file_id());
            msgUpdater.set_process_id((*iter).process_id());
            msgUpdater.set_timestamp((*iter).timestamp());
            msgUpdater.set_throughput(0.0);
            msgUpdater.set_transferred(0.0);
            ThreadSafeList::get_instance().updateMsg(msgUpdater);

            if ((*iter).transfer_status().compare("UPDATE") != 0)
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job id:" << (*iter).job_id()
                << "\nFile id: " << (*iter).file_id()
                << "\nPid: " << (*iter).process_id()
                << "\nState: " << (*iter).transfer_status()
                << "\nSource: " << (*iter).source_se()
                << "\nDest: " << (*iter).dest_se() << commit;

                updateDatabase((*iter));
            }
        }
        catch (const boost::filesystem::filesystem_error& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << e.what() << commit;
        }
        catch (std::exception& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << ex.what() << commit;
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << commit;
        }
    } //end for
}

} // end namespace server
} // end namespace fts3
