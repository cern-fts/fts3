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


#include <map>
#include <ctime>
#include <ranges>

#include "common/Uri.h"
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"
#include "server/common/DrainMode.h"

#include "FetchStaging.h"
#include "qos-daemon/task/BringOnlineTask.h"
#include "qos-daemon/task/HttpBringOnlineTask.h"
#include "qos-daemon/task/PollTask.h"
#include "qos-daemon/task/HttpPollTask.h"


void FetchStaging::fetch()
{
    stagingSchedulingInterval = fts3::config::ServerConfig::instance().get<boost::posix_time::time_duration>("StagingSchedulingInterval");
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchStaging starting (interval: "
                                    << stagingSchedulingInterval.total_seconds() << "s)" << commit;

    try {
        recoverStartedTasks();
    } catch (BaseException& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchStaging: " << e.what() << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchStaging Fatal error (unknown origin)" << commit;
    }

    while (!boost::this_thread::interruption_requested()) {
        try {
            if (fts3::server::DrainMode::instance()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more FetchStaging for this instance!" << commit;
                continue;
            }

            std::vector<StagingOperation> stagingOperations;
            time_t start = time(0);
            db::DBSingleton::instance().getDBObjectInstance()->getFilesForStaging(stagingOperations);
            time_t end = time(0);

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"FetchStaging\" "
                                            << "func=\"fetch\" "
                                            << "DBcall=\"getFilesForStaging\" "
                                            << "time=\"" << end - start << "\" "
                                            << "stagingOperations=\"" << stagingOperations.size() << "\""
                                            << commit;

            startBringonlineTasks(stagingOperations);
        } catch (const std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchStaging " << e.what() << commit;
        } catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchStaging interruption requested" << commit;
            break;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchStaging Fatal error (unknown origin)" << commit;
        }

        boost::this_thread::sleep(stagingSchedulingInterval);
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchStaging exiting" << commit;
}

void FetchStaging::recoverStartedTasks() const
{
    std::vector<StagingOperation> startedStagingOps;

    try {
        const time_t start = time(0);
        db::DBSingleton::instance().getDBObjectInstance()->getAlreadyStartedStaging(startedStagingOps);
        const time_t end = time(0);

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"FetchStaging\" "
                                        << "func=\"recoverStartedTasks\" "
                                        << "DBcall=\"getAlreadyStartedStaging\" "
                                        << "time=\"" << end - start << "\" "
                                        << "startedStagingOps=\"" << startedStagingOps.size() << "\""
                                        << commit;
    } catch (UserError const & ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
    }

    std::map<std::string, std::unique_ptr<StagingContext>> tasks;

    for (const auto& op: startedStagingOps) {
        auto task = tasks.find(op.token);

        if (task == tasks.end()) {
            tasks.try_emplace(op.token, StagingContext::createStagingContext(QoSServer::instance(), op));
        } else {
            auto& context = task->second;
            context->add(op);
        }
    }

    for (auto& [token, context]: tasks) {
        schedulePollTask(*context, token);
    }
}

void FetchStaging::startBringonlineTasks(const std::vector<StagingOperation>& stagingOperations) const
{
    const auto stagingBulkSize = fts3::config::ServerConfig::instance().get<uint64_t>("StagingBulkSize");
    std::map<GroupByType, std::unique_ptr<StagingContext>> tasks;

    for (const auto& op: stagingOperations) {
        std::string storage = Uri::parse(op.surl).getSeName();
        GroupByType key(op.credId, storage, op.spaceToken);
        auto task = tasks.find(key);

        if (task == tasks.end()) {
            tasks.try_emplace(key, StagingContext::createStagingContext(QoSServer::instance(), op));
            continue;
        }

        auto& context = task->second;

        if (context->getNbUrls() >= stagingBulkSize) {
            scheduleBringonlineTask(*context);
            tasks.erase(key);
            tasks.try_emplace(key, StagingContext::createStagingContext(QoSServer::instance(), op));
        } else {
            context->add(op);
        }
    }

    for (auto& context: tasks | std::views::values) {
        scheduleBringonlineTask(*context);
    }
}

void FetchStaging::scheduleBringonlineTask(StagingContext& context) const
{
    if (context.updateStateToStarted()) {
        try {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Starting BringonlineTask [storage=" << context.getStorageEndpoint()
                                            << " / " << context.getNbUrls() << " files]: "
                                            << context.getLogMsg() << commit;

            std::string protocol = context.getStorageProtocol();
            if (protocol == "http" || protocol == "https" || protocol == "dav" || protocol == "davs") {
                threadpool.start(new HttpBringOnlineTask(std::move(dynamic_cast<HttpStagingContext&&>(context))));
            } else {
                threadpool.start(new BringOnlineTask(std::move(context)));
            }
        } catch (UserError const & ex) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
        }
    }
}

void FetchStaging::schedulePollTask(StagingContext& context, const std::string& token) const
{
    try {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Recovering PollingTask [storage=" << context.getStorageEndpoint()
                                        << " / " << context.getNbUrls() << " files]: "
                                        << context.getLogMsg() << commit;

        std::string protocol = context.getStorageProtocol();
        if (protocol == "http" || protocol == "https" || protocol == "dav" || protocol == "davs") {
            threadpool.start(new HttpPollTask(std::move(dynamic_cast<HttpStagingContext&>(context)), token));
        } else {
            threadpool.start(new PollTask(std::move(context), token));
        }
    } catch (UserError const & ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception, continuing to see..." << commit;
    }
}
