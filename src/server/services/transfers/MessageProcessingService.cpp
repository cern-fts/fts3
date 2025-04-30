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


MessageProcessingService::MessageProcessingService(): BaseService("MessageProcessingService"),
    consumer(ServerConfig::instance().get<std::string>("MessagingDirectory")),
    producer(ServerConfig::instance().get<std::string>("MessagingDirectory"))
{
    messages.reserve(600);
}


void MessageProcessingService::runService()
{
    namespace fs = boost::filesystem;
    auto msgCheckInterval = ServerConfig::instance().get<boost::posix_time::time_duration>("MessagingConsumeInterval");
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "MessageProcessingService interval: " << msgCheckInterval.total_seconds() << "s" << commit;

    while (!boost::this_thread::interruption_requested())
    {
        updateLastRunTimepoint();

        try {
            if (boost::this_thread::interruption_requested() && messages.empty() && messagesLog.empty())
            {
                break;
            }

            // if conn to the db is lost, do not retrieve state, save it for later
            // use one fast query
            try {
                db::DBSingleton::instance().getDBObjectInstance()->getDrain();
            } catch (...) {
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
                handleOtherMessages(messages);
                handleUpdateMessages(messages);
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

            // update progress vector
            if (consumer.runConsumerStall(messagesUpdater) != 0)
            {
                char buffer[128] = { 0 };
                FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Could not get the updater messages:" << strerror_r(errno, buffer, sizeof(buffer)) << commit;
                continue;
            }

            if (!messagesUpdater.empty())
            {
                std::vector<fts3::events::MessageUpdater>::iterator iterUpdater;
                for (iterUpdater = messagesUpdater.begin(); iterUpdater != messagesUpdater.end(); ++iterUpdater)
                {
                    std::string job = (*iterUpdater).job_id().substr(0, 36);
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
        } catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested in SupervisorService!" << commit;
            dumpMessages();
            break;
        } catch (const std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue thrown exception: " << e.what() << commit;
            dumpMessages();
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue thrown unhandled exception!" << commit;
            dumpMessages();
        }

        boost::this_thread::sleep(msgCheckInterval);
    }
}


void MessageProcessingService::performUpdateMessageDbChange(const fts3::events::Message& msg)
{
    try
    {
        // process only UPDATE messages
        if (msg.transfer_status().compare("UPDATE") != 0)
            return;

        std::ostringstream internal_params;
        internal_params << "nostreams:" << static_cast<int>(msg.nostreams())
                        << ",timeout:" << static_cast<int>(msg.timeout())
                        << ",buffersize:" << static_cast<int>(msg.buffersize());

        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Update message job_id=" << msg.job_id()
                                         << " file_id=" << msg.file_id()
                                         << " file_size=" << msg.filesize()
                                         << " file_params=" << internal_params.str()
                                         << commit;

        db::DBSingleton::instance().getDBObjectInstance()->updateProtocol(std::vector<events::Message>{msg});
    }
    catch (const std::exception& e)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue performUpdateMessageDbChange thrown exception " << e.what() << commit;

        // Encountered unexpected DB error. Terminate all files of a given job
        if (isUnrecoverableErrorMessage(e.what())) {
            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Attempted database change with invalid values: " << e.what() << commit;
            db::DBSingleton::instance().getDBObjectInstance()->terminateReuseProcess(
                    msg.job_id(), msg.process_id(), "Database change failed due to invalid values", true);
            return;
        }

        producer.runProducerStatus(msg);
    }
    catch (...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue performUpdateMessageDbChange thrown exception" << commit;
        producer.runProducerStatus(msg);
    }
}


void MessageProcessingService::performOtherMessageDbChange(const fts3::events::Message& msg)
{
    try
    {
        // do not process UPDATE messages
        if (msg.transfer_status().compare("UPDATE") == 0)
            return;

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job id: " << msg.job_id()
                                        << "\nFile id: " << msg.file_id()
                                        << "\nPid: " << msg.process_id()
                                        << "\nState: " << msg.transfer_status()
                                        << "\nSource: " << msg.source_se()
                                        << "\nDest: " << msg.dest_se() << commit;

        if (msg.transfer_status().compare("FINISHED") == 0) {
            FTS3_COMMON_LOGGER_NEWLOG(PROF) << "[profiling:transfer]"
                                            << " file_id=" << msg.file_id()
                                            << " timestamp=" << msg.gfal_perf_timestamp() / 1000
                                            << " inst_throughput=" << msg.instantaneous_throughput()
                                            << " dif_transferred=" << msg.transferred_since_last_ping()
                                            << " source_se=" << msg.source_se()
                                            << " dest_se=" << msg.dest_se()
                                            << commit;
        }


        if (msg.transfer_status().compare("FINISHED") == 0 ||
            msg.transfer_status().compare("FAILED") == 0 ||
            msg.transfer_status().compare("CANCELED") == 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO)
                << "Removing job from monitoring list " << msg.job_id() << " " << msg.file_id()
                << commit;
            ThreadSafeList::get_instance().removeFinishedTr(msg.job_id(), msg.file_id());
        }

        if (msg.transfer_status().compare("FAILED") == 0)
        {
            try
            {
                // multiple replica files belonging to a job will not be retried
                int retry = db::DBSingleton::instance().getDBObjectInstance()->getRetry(msg.job_id());

                if (msg.retry() == true && retry > 0 && msg.file_id() > 0)
                {
                    int retryTimes = db::DBSingleton::instance().getDBObjectInstance()->getRetryTimes(msg.job_id(), msg.file_id());

                    if (retryTimes <= retry - 1)
                    {
                        db::DBSingleton::instance().getDBObjectInstance()->setRetryTransfer(
                            msg.job_id(), msg.file_id(), retryTimes + 1,
                            msg.transfer_message(), msg.log_path(), msg.errcode());
                        return;
                    }
                }
            }
            catch (std::exception& e)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue performOtherMessageDbChange throw exception when set retry " << e.what() << commit;
            }
            catch (...)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue performOtherMessageDbChange throw exception when set retry " << commit;
            }
        }

        // session reuse process died or terminated unexpected. Terminate all files of a given job
        if (isUnrecoverableErrorMessage(msg.transfer_message()))
        {
            db::DBSingleton::instance().getDBObjectInstance()->terminateReuseProcess(
                msg.job_id(), msg.process_id(), msg.transfer_message(), false);
        }

        // update file and job state
        boost::tuple<bool, std::string> updated = db::DBSingleton::instance()
            .getDBObjectInstance()->updateTransferStatus(
                msg.job_id(), msg.file_id(), msg.process_id(),
                msg.transfer_status(), msg.transfer_message(),
                msg.filesize(), msg.time_in_secs(), msg.throughput(),
                msg.retry(), msg.file_metadata());

        db::DBSingleton::instance().getDBObjectInstance()->updateJobStatus(
            msg.job_id(), msg.transfer_status());

        if (!updated.get<0>() && msg.transfer_status() != "CANCELED") {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Entry in the database not updated for "
                << msg.job_id() << " " << msg.file_id()
                << ". Probably already in a different terminal state. Tried to set "
                << msg.transfer_status() << " over " << updated.get<1>() << commit;
        }
        else if (!msg.job_id().empty() && msg.file_id() > 0) {
            SingleTrStateInstance::instance().sendStateMessage(msg.job_id(), msg.file_id());
        }
    }
    catch (const std::exception& e)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue performOtherMessageDbChange throw exception " << e.what() << commit;

        // Encountered unexpected DB error. Terminate all files of a given job
        if (isUnrecoverableErrorMessage(e.what())) {
            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Attempted database change with invalid values: " << e.what() << commit;
            db::DBSingleton::instance().getDBObjectInstance()->terminateReuseProcess(
                    msg.job_id(), msg.process_id(), "Database change failed due to invalid values", true);
            return;
        }

        producer.runProducerStatus(msg);
    }
    catch (...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue performOtherMessageDbChange throw exception" << commit;
        producer.runProducerStatus(msg);
    }
}


void MessageProcessingService::handleUpdateMessages(const std::vector<fts3::events::Message>& messages)
{
    for (auto iter = messages.begin(); iter != messages.end(); ++iter)
    {
        try
        {
            if (boost::this_thread::interruption_requested())
            {
                dumpMessages();
                return;
            }

            if ((*iter).transfer_status().compare("UPDATE") == 0)
            {
                performUpdateMessageDbChange(*iter);
            }
        }
        catch (const boost::filesystem::filesystem_error& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Caught exception related to message dumping: " << e.what() << commit;
        }
        catch (const std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << e.what() << commit;
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << commit;
        }
    }
}


void MessageProcessingService::handleOtherMessages(const std::vector<fts3::events::Message>& messages)
{
    fts3::events::MessageUpdater msgUpdater;

    for (auto iter = messages.begin(); iter != messages.end(); ++iter)
    {
        try
        {
            if (boost::this_thread::interruption_requested())
            {
                dumpMessages();
                return;
            }

            msgUpdater.set_job_id((*iter).job_id());
            msgUpdater.set_file_id((*iter).file_id());
            msgUpdater.set_process_id((*iter).process_id());
            msgUpdater.set_timestamp((*iter).timestamp());
            msgUpdater.set_throughput(0.0);
            msgUpdater.set_instantaneous_throughput(0.0);
            msgUpdater.set_transferred(0);
            ThreadSafeList::get_instance().updateMsg(msgUpdater);

            if ((*iter).transfer_status().compare("UPDATE") != 0)
            {
                performOtherMessageDbChange(*iter);
            }
        }
        catch (const boost::filesystem::filesystem_error& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Caught exception related to message dumping: " << e.what() << commit;
        }
        catch (const std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << e.what() << commit;
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << commit;
        }
    }
}


void MessageProcessingService::dumpMessages()
{
    try
    {
        for (auto iter = messages.begin(); iter != messages.end(); ++iter)
        {
            producer.runProducerStatus(*iter);
        }

        for (auto iterLog = messagesLog.begin(); iterLog != messagesLog.end(); ++iterLog)
        {
            auto messageLog = (*iterLog).second;
            producer.runProducerLog(messageLog);
        }
    }
    catch (const boost::filesystem::filesystem_error& e)
    {
        FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Caught exception while dumping messages: " << e.what() << commit;
    }
}


bool MessageProcessingService::isUnrecoverableErrorMessage(const std::string& errmsg)
{
    return errmsg.find("Transfer terminate handler called") != std::string::npos ||
           errmsg.find("Transfer process died") != std::string::npos ||
           errmsg.find("because it was stalled") != std::string::npos ||
           errmsg.find("canceled by the user") != std::string::npos ||
           errmsg.find("undefined symbol") != std::string::npos ||
           errmsg.find("canceled because it was not responding") != std::string::npos ||
           errmsg.find("Out of range value") != std::string::npos;
}

} // end namespace server
} // end namespace fts3
