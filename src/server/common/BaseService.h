/*
 * Copyright (c) CERN 2015
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

#include <mutex>
#include <chrono>
#include <boost/noncopyable.hpp>
#include <boost/thread/exceptions.hpp>
#include "common/Logger.h"

namespace fts3 {
namespace server {

/// Base class for all services
/// Intended to be able to treat all of them with the same API
class BaseService: public boost::noncopyable {
protected:
    BaseService(const std::string& serviceName): serviceName(serviceName) {
        lastRun = std::chrono::steady_clock::now();
    }

    void setServiceName(const std::string& newServiceName) {
        serviceName = newServiceName;
    }

public:
    virtual ~BaseService() {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << getServiceName() << " destroyed" << fts3::common::commit;
    };

    std::string getServiceName() {
        return serviceName;
    }

    virtual void runService() = 0;

    virtual void operator() () {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Starting " << getServiceName() << fts3::common::commit;

        try {
            runService();
        } catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Requested interruption of " << getServiceName()
                << fts3::common::commit;
        } catch (const std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unhandled exception for " << getServiceName()
                << ": " << e.what() << fts3::common::commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unhandled unknown exception for "
                << getServiceName() << fts3::common::commit;
        }

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Exiting " << getServiceName() << fts3::common::commit;
    }

    void updateLastRunTimepoint() {
        std::unique_lock lock(mutex);
        lastRun = std::chrono::steady_clock::now();
    }

    std::chrono::steady_clock::time_point getLastRunTimepoint() {
        std::unique_lock lock(mutex);
        return lastRun;
    }

private:
    std::string serviceName; ///< The service name
    std::mutex mutex; ///< Mutex for lastRun timepoint
    std::chrono::steady_clock::time_point lastRun; ///< Timepoint for the last "runService()" execution
};

} // namespace server
} // namespace fts3
