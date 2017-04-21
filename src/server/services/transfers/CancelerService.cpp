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

#include "CancelerService.h"

#include <signal.h>

#include <boost/filesystem.hpp>

#include "common/Logger.h"
#include "config/ServerConfig.h"
#include "server/DrainMode.h"
#include "SingleTrStateInstance.h"
#include "ThreadSafeList.h"


using namespace fts3::common;
using fts3::config::ServerConfig;


namespace fts3 {
namespace server {

extern time_t stallRecords;


CancelerService::CancelerService(): BaseService("CancelerService")
{
}


CancelerService::~CancelerService()
{
}


void CancelerService::markAsStalled()
{
    auto db = DBSingleton::instance().getDBObjectInstance();
    const boost::posix_time::seconds timeout(ServerConfig::instance().get<int>("CheckStalledTimeout"));

    std::vector<fts3::events::MessageUpdater> messages;
    messages.reserve(500);
    ThreadSafeList::get_instance().checkExpiredMsg(messages, timeout);

    if (!messages.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Reaping stalled transfers" << commit;

        boost::filesystem::path p(ServerConfig::instance().get<std::string>("MessagingDirectory"));
        boost::filesystem::space_info s = boost::filesystem::space(p);
        bool diskFull = (s.free <= 0 || s.available <= 0);
        std::stringstream reason;

        if (diskFull) {
            reason << "No space left on device";
        }
        else {
            reason << "No FTS server has updated the transfer status the last "
                << timeout.total_seconds() << " seconds"
                << ". Probably stalled";
        }

        for (auto i = messages.begin(); i != messages.end(); ++i) {
            kill(i->process_id(), SIGKILL);
            boost::tuple<bool, std::string> updated = db->updateTransferStatus(i->job_id(), i->file_id(), 0,
                "FAILED", reason.str(), i->process_id(),
                0, 0, false);
            db->updateJobStatus(i->job_id(), "FAILED", i->process_id());

            if (updated.get<0>()) {
                SingleTrStateInstance::instance().sendStateMessage(i->job_id(), i->file_id());
            }
            else {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Tried to mark as stalled, but already terminated: "
                    << i->job_id() << "/" << i->file_id() << " "  << updated.get<1>() << commit;
            }
        }
        ThreadSafeList::get_instance().deleteMsg(messages);
    }
}


void CancelerService::killCanceledByUser()
{
    std::vector<int> requestIDs;
    DBSingleton::instance().getDBObjectInstance()->getCancelJob(requestIDs);
    if (!requestIDs.empty())
    {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Killing transfers canceled by the user" << commit;
        killRunningJob(requestIDs);
    }
}


void CancelerService::applyQueueTimeouts()
{
    std::vector<std::string> jobs;
    DBSingleton::instance().getDBObjectInstance()->setToFailOldQueuedJobs(jobs);
    if (!jobs.empty())
    {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Applying queue timeouts" << commit;
        std::vector<std::string>::const_iterator iter2;
        for (iter2 = jobs.begin(); iter2 != jobs.end(); ++iter2)
        {
            SingleTrStateInstance::instance().sendStateMessage((*iter2), -1);
        }
        jobs.clear();
    }
}


void CancelerService::applyActiveTimeouts()
{
    std::vector<TransferFile> stalled;
    auto db = DBSingleton::instance().getDBObjectInstance();

    db->reapStalledTransfers(stalled);

    std::vector<fts3::events::MessageUpdater> messages;

    for (auto i = stalled.begin(); i != stalled.end(); ++i) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Killing pid:" << i->pid
            << ", jobid:" << i->jobId << ", fileid:" << i->fileId
            << " because it was stalled" << commit;
        kill(i->pid, SIGKILL);
        db->updateTransferStatus(i->jobId, i->fileId, 0.0,
            "FAILED", "Transfer has been forced-killed because it was stalled",
            i->pid, 0, 0, false);
        db->updateJobStatus(i->jobId, "FAILED", i->pid);
        SingleTrStateInstance::instance().sendStateMessage(i->jobId, i->fileId);

        fts3::events::MessageUpdater msg;
        msg.set_job_id(i->jobId);
        msg.set_file_id(i->fileId);

        messages.emplace_back(msg);
    }
    ThreadSafeList::get_instance().deleteMsg(messages);
}


/// Get a list of processes belonging to this host from the DB
/// To be called on start
static void recoverProcessesFromDb()
{
    auto actives = DBSingleton::instance().getDBObjectInstance()->getActiveInHost(getFullHostname());
    for (auto i = actives.begin(); i != actives.end(); ++i) {
        const fts3::events::MessageUpdater &msg = *i;
        ThreadSafeList::get_instance().push_back(*i);
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Adding to watchlist from DB: "
            << msg.job_id() << " / " << msg.file_id() << commit;
    }
}


void CancelerService::runService()
{
    unsigned int counterActiveTimeouts = 0;
    unsigned int counterQueueTimeouts = 0;
    unsigned int counterCanceled = 0;

    int cancelInterval = ServerConfig::instance().get<int>("CancelCheckInterval");
    int queueTimeoutInterval = ServerConfig::instance().get<int>("QueueTimeoutCheckInterval");
    int activeTimeoutInterval = ServerConfig::instance().get<int>("ActiveTimeoutCheckInterval");
    bool checkStalledTransfers = ServerConfig::instance().get<bool>("CheckStalledTransfers");

    recoverProcessesFromDb();


    while (!boost::this_thread::interruption_requested())
    {
        stallRecords = time(0);
        try
        {
            //if we drain a host, no need to check if url_copy are reporting being alive
            if (DrainMode::instance())
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more checking url_copy for this instance!" << commit;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }

            markAsStalled();

            if (boost::this_thread::interruption_requested())
                return;

            /*also get jobs which have been canceled by the client*/
            counterCanceled++;
            if (cancelInterval > 0 && counterCanceled >= cancelInterval)
            {
                killCanceledByUser();
                counterCanceled = 0;
            }

            if (boost::this_thread::interruption_requested())
                return;

            /*force-fail stalled ACTIVE transfers*/
            if (checkStalledTransfers && activeTimeoutInterval > 0) {
                counterActiveTimeouts++;
                if (counterActiveTimeouts >= activeTimeoutInterval)
                {
                    applyActiveTimeouts();
                    counterActiveTimeouts = 0;
                }
            }

            if (boost::this_thread::interruption_requested())
                return;

            /*set to fail all old queued jobs which have exceeded max queue time*/
            counterQueueTimeouts++;
            if (queueTimeoutInterval > 0 && counterQueueTimeouts >= queueTimeoutInterval)
            {
                applyQueueTimeouts();
                counterQueueTimeouts = 0;
            }
        }
        catch (boost::thread_interrupted&)
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested" << commit;
            break;
        }
        catch (const std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "CancelerService caught exception " << e.what() << commit;
            boost::this_thread::sleep(boost::posix_time::seconds(10));
            counterActiveTimeouts = 0;
            counterQueueTimeouts = 0;
            counterCanceled = 0;
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "CancelerService caught unknown exception" << commit;
            boost::this_thread::sleep(boost::posix_time::seconds(10));
            counterActiveTimeouts = 0;
            counterQueueTimeouts = 0;
            counterCanceled = 0;
        }
        boost::this_thread::sleep(boost::posix_time::seconds(1));
    }
}


void CancelerService::killRunningJob(const std::vector<int>& pids)
{
    for (auto iter = pids.begin(); iter != pids.end(); ++iter)
    {
        int pid = *iter;
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Canceling and killing running processes: " << pid << commit;
        kill(pid, SIGTERM);
    }
}

} // end namespace server
} // end namespace fts3
