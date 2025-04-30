/*
 * Copyright (c) CERN 2013-2024
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

#include <chrono>
#include <ctime>
#include <shared_mutex>

#include "common/Singleton.h"
#include "server/common/BaseService.h"

namespace fts3 {
namespace server {


class HeartBeat: public BaseService
{
public:
    HeartBeat(std::string processName);
    virtual ~HeartBeat() = default;

    virtual void runService();

    /**
     * Returns whether this node is the first alphabetical node
     * running the service in "t_hosts" (not in draining)
     *
     * @param bypassDraining When true, consider between all nodes,
     *                       including those in draining
     */
    bool isLeadNode(bool bypassDraining = false) const;

    /**
     * Register services which should be watched to not become inactive.
     * On registration, a grace time and a graceful abort function are provided.
     *
     * If a service is inactive for more than the grace time, the graceful
     * abort function is called and the entire process is abruptly stopped.
     */
    void registerWatchedService(const std::shared_ptr<BaseService>& service, int graceTime,
                                const std::function<void()>& gracefulAbortFun)
    {
        std::scoped_lock lock(mxWatchedServices);
        watchedServices.emplace(service, WatchData(graceTime, gracefulAbortFun));
    }

private:
    struct WatchData {
        explicit WatchData(const int graceTime,
                          const std::function<void()>& fun_gracefulAbort):
        graceTime(graceTime),
        fun_gracefulAbort(fun_gracefulAbort)
        {
            last = std::chrono::steady_clock::now();
        }

        int graceTime;
        const std::function<void()> fun_gracefulAbort;
        std::chrono::time_point<std::chrono::steady_clock> last;
    };

    /**
     * Check all watched services if they became inactive.
     * If an inactive service is found, (gracefully) abort the whole process.
     */
    void criticalServiceExpired();

    unsigned index; ///< Current node's order in the alphabetical sorting
    unsigned count; ///< Total number of nodes running the same service
    unsigned start; ///< Start hash range for this index
    unsigned end;   ///< End hash range for this index

    std::string processName;
    std::shared_mutex mxWatchedServices;
    std::map<std::shared_ptr<BaseService>, WatchData> watchedServices;
};

} // end namespace server
} // end namespace fts3
