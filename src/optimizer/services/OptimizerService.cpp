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

#include <chrono>
#include <sstream>

#include "common/ThreadPool.h"
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"

#include "OptimizerService.h"
#include "OptimizerExecutor.h"
#include "OptimizerDataSource.h"
#include "DbOptimizerDataSource.h"

namespace fts3 {
namespace optimizer {

using optimizer::OptimizerExecutor;
using optimizer::OptimizerDataSource;
using optimizer::DbOptimizerDataSource;

using namespace fts3::config;
using namespace fts3::common;
using namespace fts3::server;

// At the moment DbOptimizerDataSource is the only implementation of OptimizerDataSource
// In the future this factory can be extended to return different types of OptimizerDataSource implementations
class OptimizerDataSourceFactory {
public:
    static std::unique_ptr<OptimizerDataSource> getDataSource() {
        return std::make_unique<DbOptimizerDataSource>();
    }
};

// At the moment OptimizerNotifier is the only implementation of OptimizerCallbacks
// The factory is used to return an instance of OptimizerNotifier if monitoring messages are enabled in the config file
class OptimizerCallbacksFactory {
public:
    static std::unique_ptr<OptimizerCallbacks> getOptimizerCallbacks() {
        const auto enabled = ServerConfig::instance().get<bool>("MonitoringMessaging");
        const auto messageDir = ServerConfig::instance().get<std::string>("MessagingDirectory");

        if (!enabled) {
            return nullptr;
        }

        return std::make_unique<OptimizerNotifier>(messageDir);
    }
};


OptimizerService::OptimizerService(const std::shared_ptr<HeartBeat>& heartBeat):
    BaseService("OptimizerService"),
    heartBeat(heartBeat)
{
    optimizerPoolSize = ServerConfig::instance().get<int>("OptimizerThreadPool");
}


void OptimizerService::runService()
{
    typedef boost::posix_time::time_duration TDuration;
    const auto optimizerInterval = ServerConfig::instance().get<TDuration>("OptimizerInterval");

    while (!boost::this_thread::interruption_requested()) {
        try {
            if (heartBeat->isLeadNode()) {
                optimizeAllPairs();
            } else {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer: Not the leading node..." << commit;
            }
        } catch (std::exception &e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in main process OptimizerService: " << e.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in main process OptimizerService!" << commit;
        }

        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Optimizer: Going to sleep for " << optimizerInterval.total_seconds() << "s" << commit;
        boost::this_thread::sleep(optimizerInterval);
    }
}

void OptimizerService::optimizeAllPairs() {

    ThreadPool<OptimizerExecutor> execPool(optimizerPoolSize); // Run optimizer for each pair in a separate thread
    auto db = db::DBSingleton::instance().getDBObjectInstance();

    // Read all Optimizer configurations from the config file
    typedef boost::posix_time::time_duration TDuration;
    auto optimizerSteadyInterval = ServerConfig::instance().get<TDuration>("OptimizerSteadyInterval");
    auto maxNumberOfStreams = ServerConfig::instance().get<int>("OptimizerMaxStreams");
    auto maxSuccessRate = ServerConfig::instance().get<int>("OptimizerMaxSuccessRate");
    auto lowSuccessRate = ServerConfig::instance().get<int>("OptimizerLowSuccessRate");
    auto baseSuccessRate = ServerConfig::instance().get<int>("OptimizerBaseSuccessRate");

    auto emaAlpha = ServerConfig::instance().get<double>("OptimizerEMAAlpha");
    auto increaseStep = ServerConfig::instance().get<int>("OptimizerIncreaseStep");
    auto increaseAggressiveStep = ServerConfig::instance().get<int>("OptimizerAggressiveIncreaseStep");
    auto decreaseStep = ServerConfig::instance().get<int>("OptimizerDecreaseStep");

    try {
        // Just get the all the active queues
        std::list<Pair> pairs = db->getActivePairs();
        // Make sure the order is always the same
        // See FTS-1094
        pairs.sort();

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer run start: " << pairs.size() << " pairs to be optimized" << commit;
        const auto start = std::chrono::steady_clock::now();

        // For each par create an optimizer task and run it in the thread pool
        for (const auto& pair: pairs) {
            auto *exec = new OptimizerExecutor(OptimizerDataSourceFactory::getDataSource(),
                                               OptimizerCallbacksFactory::getOptimizerCallbacks(),
                                               pair);

            exec->setSteadyInterval(optimizerSteadyInterval);
            exec->setMaxNumberOfStreams(maxNumberOfStreams);
            exec->setMaxSuccessRate(maxSuccessRate);
            exec->setLowSuccessRate(lowSuccessRate);
            exec->setBaseSuccessRate(baseSuccessRate);
            exec->setEmaAlpha(emaAlpha);
            exec->setStepSize(increaseStep, increaseAggressiveStep, decreaseStep);

            execPool.start(exec);
        }

        execPool.join();

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - start).count();
        std::ostringstream elapsed_ss;
        elapsed_ss << std::fixed << std::setprecision(6) << elapsed;

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer run finish: optimized " << pairs.size() << " pairs"
                                        << " elapsed=" << elapsed_ss.str() << "s" << commit;
    } catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in OptimizerService::optimizeAllPairs" << commit;
        execPool.interrupt();
        execPool.join();
        throw;
    } catch (std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in OptimizerService::optimizeAllPairs: " << e.what() << commit;
        throw;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in OptimizerService::optimizeAllPairs!" << commit;
        throw;
    }
}

} // end namespace optimizer
} // end namespace fts3
