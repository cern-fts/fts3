/*
 * Copyright (c) CERN 2013-2025
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

#include "common/Uri.h"
#include "db/generic/SingleDbInstance.h"
#include "server/common/DrainMode.h"

#include "FetchArchiving.h"
#include "qos-daemon/task/ArchivingPollTask.h"


void FetchArchiving::fetch()
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchArchiving starting" << commit;

    // we want to be sure that this won't break out fetching thread
    try
    {
        recoverStartedTasks();
    }
    catch (BaseException& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchArchiving " << e.what() << commit;
    }
    catch (...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchArchiving Fatal error (unknown origin)" << commit;
    }

    while (!boost::this_thread::interruption_requested()) {
        try {
            if (fts3::server::DrainMode::instance()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more checking archiving files for this instance!" << commit;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }

            std::vector<ArchivingOperation> files;

            time_t start = time(0);
            db::DBSingleton::instance().getDBObjectInstance()->getFilesForArchiving(files);
            time_t end = time(0);
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"FetchArchiving\" "
                                            << "func=\"fetch\" "
                                            << "DBcall=\"getFilesForArchiving\" "
                                            << "time=\"" << end - start << "\""
                                            << commit;

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Fetched " << files.size()
                                            << " files for archiving" << commit;

            startArchivePollTasks(files);
            boost::this_thread::sleep(boost::posix_time::seconds(60));
        }
        catch (const std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchArchiving " << e.what() << commit;
        }
        catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchArchiving interruption requested" << commit;
            break;
        }
        catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchArchiving Fatal error (unknown origin)" << commit;
        }
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchArchiving exiting" << commit;
}


void FetchArchiving::recoverStartedTasks() const
{
    std::vector<ArchivingOperation> startedArchivingOps;

    try {
        // Retrieve from DB all the files with archive_start_time not null
        time_t start = time(0);
        db::DBSingleton::instance().getDBObjectInstance()->getAlreadyStartedArchiving(startedArchivingOps);
        time_t end = time(0);
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"FetchArchiving\" "
                                        << "func=\"recoverStartedTasks\" "
                                        << "DBcall=\"getAlreadyStartedArchiving\" "
                                        << "time=\"" << end - start << "\""
                                        << commit;

    }
    catch (UserError const & ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
    }
    catch(...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Recovering " << startedArchivingOps.size()
                                    << " archiving operations" << commit;

    startArchivePollTasks(startedArchivingOps);
}


void FetchArchiving::startArchivePollTasks(const std::vector<ArchivingOperation>& archivingOperations) const
{
    auto archivePollBulkSize = fts3::config::ServerConfig::instance().get<uint64_t>("ArchivePollBulkSize");
    std::map<GroupByType, ArchivingContext> tasks;

    for (const auto& op : archivingOperations) {
        // Apply grouping by credential ID and storage endpoint
        std::string storage = Uri::parse(op.surl).getSeName();
        GroupByType key(op.credId, storage);
        // Look up for a task with a current key
        auto task_it = tasks.find(key);

        if (task_it == tasks.end()) {
            // No task exist for this key so insert a new one in the tasks container
            tasks.try_emplace(key, QoSServer::instance(), op);
        }
        // Task already exists. See if it contains enough URL's to start an ArchivePollTask on the server thread pool
        else if (task_it->second.getNbUrls() >= archivePollBulkSize) {
            // Start ArchivePollTasks on the server thread pool
            scheduleArchivePollTask(task_it->second);
            // Remove key from the task container as it has already been started
            tasks.erase(key);
            // Create a new task with the current operation
            tasks.try_emplace(key, QoSServer::instance(), op);
        } else {
            // There are not yet enough urls on this task to start a ArchivePollTask so just add current operation
            task_it->second.add(op);
        }
    }

    // Start ArchivePollTasks for the remaining tasks that don't have enough urls to fill a batch
    for (const auto& task : tasks) {
        scheduleArchivePollTask(task.second);
    }
}


void FetchArchiving::scheduleArchivePollTask(const ArchivingContext &context) const {
    try {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Starting archiving task for storage " << context.getStorageEndpoint()
                                        << " with " << context.getNbUrls() << " files : "
                                        <<  context.getLogMsg() << commit;

        threadpool.start(new ArchivingPollTask(context));
    }
    catch (UserError const & ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
    }
    catch(...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
    }
}
