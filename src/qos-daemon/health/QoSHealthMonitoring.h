/*
 * Copyright (c) CERN 2022
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

#include "common/ThreadSafeCounter.h"
#include "config/ServerConfig.h"

using namespace fts3::config;

class QoSHealthMonitoring
{
public:
    QoSHealthMonitoring() : stagingTaskCounter()
    {
        qosMonitoringInterval = ServerConfig::instance().get<boost::posix_time::time_duration>("QosMonitoringInterval");
    }

    QoSHealthMonitoring(const QoSHealthMonitoring& copy) = delete;

    QoSHealthMonitoring(QoSHealthMonitoring&& copy) = delete;

    ThreadSafeCounter& getStagingTaskCounter() {
        return stagingTaskCounter;
    }

    ThreadSafeCounter& getHttpStagingTaskCounter() {
        return httpStagingTaskCounter;
    }

    ThreadSafeCounter& getArchivingTaskCounter() {
        return archivingTaskCounter;
    }

    ThreadSafeCounter& getDeletionTaskCounter() {
        return deletionTaskCounter;
    }

    void run()
    {
        while (!boost::this_thread::interruption_requested())
        {
            boost::this_thread::sleep(qosMonitoringInterval);
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "StagingTasks running: " << stagingTaskCounter.getCounter() << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "HttpStagingTasks running: " << httpStagingTaskCounter.getCounter() << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "ArchivingTasks running: " << archivingTaskCounter.getCounter() << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DeletionTasks running: " << deletionTaskCounter.getCounter() << commit;

        }
    }

private:
    boost::posix_time::time_duration qosMonitoringInterval;
    // Tasks counters
    ThreadSafeCounter stagingTaskCounter;
    ThreadSafeCounter httpStagingTaskCounter;
    ThreadSafeCounter archivingTaskCounter;
    ThreadSafeCounter deletionTaskCounter;
};
