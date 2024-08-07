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

#include  <config/ServerConfig.h>
#include "OptimizerService.h"
#include "OptimizerExecutor.h"
#include "OptimizerDataSource.h"
#include "DbOptimizerDataSource.h"
#include "common/ThreadPool.h"

#include "db/generic/SingleDbInstance.h"


namespace fts3 {
namespace server {

using optimizer::OptimizerExecutor;
using optimizer::OptimizerDataSource;
using optimizer::DbOptimizerDataSource;
using namespace fts3::common;

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
        const auto enabled = config::ServerConfig::instance().get<bool>("MonitoringMessaging");
        const auto messageDir = config::ServerConfig::instance().get<std::string>("MessageDirectory");

        if (!enabled) {
            return nullptr;
        }

        return std::make_unique<OptimizerNotifier>(messageDir);
    }
};


OptimizerService::OptimizerService(HeartBeat *beat): BaseService("OptimizerService"), beat(beat)
{
}


void OptimizerService::runService()
{
    typedef boost::posix_time::time_duration TDuration;
    const auto optimizerInterval = config::ServerConfig::instance().get<TDuration>("OptimizerInterval");

    while (!boost::this_thread::interruption_requested()) {
        try {
            if (beat->isLeadNode()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer run: Start optimizer cycle" << commit;
                optimizeAllPairs();
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer run: Finished optimizer cycle" << commit;
            } else {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer: Not the leading node..." << commit;
            }
        } catch (std::exception &e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread OptimizerService " << e.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread OptimizerService unknown" << commit;
        }

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer: Going to sleep for " << optimizerInterval.seconds() << commit;
        boost::this_thread::sleep(optimizerInterval);
    }
}

void OptimizerService::optimizeAllPairs() {

    ThreadPool<OptimizerExecutor> execPool(10); // Run optimizer for each link in a thread pool
    auto db = db::DBSingleton::instance().getDBObjectInstance();

    // Read all Optimizer configurations from the config file
    typedef boost::posix_time::time_duration TDuration;
    auto optimizerSteadyInterval = config::ServerConfig::instance().get<TDuration>("OptimizerSteadyInterval");
    auto maxNumberOfStreams = config::ServerConfig::instance().get<int>("OptimizerMaxStreams");
    auto maxSuccessRate = config::ServerConfig::instance().get<int>("OptimizerMaxSuccessRate");
    auto lowSuccessRate = config::ServerConfig::instance().get<int>("OptimizerLowSuccessRate");
    auto baseSuccessRate = config::ServerConfig::instance().get<int>("OptimizerBaseSuccessRate");

    auto emaAlpha = config::ServerConfig::instance().get<double>("OptimizerEMAAlpha");
    auto increaseStep = config::ServerConfig::instance().get<int>("OptimizerIncreaseStep");
    auto increaseAggressiveStep = config::ServerConfig::instance().get<int>("OptimizerAggressiveIncreaseStep");
    auto decreaseStep = config::ServerConfig::instance().get<int>("OptimizerDecreaseStep");

    try {
        // Just get the all the active queues
        std::list<Pair> pairs = db->getActivePairs();
        // Make sure the order is always the same
        // See FTS-1094
        pairs.sort();

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer run: found " << pairs.size() << " pairs to be optimized" << commit;

        // For each par create an optimizer task and run it in the thread pool
        for (const auto& pair : pairs) {
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
        FTS3_COMMON_LOGGER_NEWLOG(INFO) <<"Optimizer run: optimized " << pairs.size() << " pairs" << commit;
    }
    catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in OptimizerService::optimizeAllPairs" << commit;
        execPool.interrupt();
        execPool.join();
        throw;
    }
    catch (std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in OptimizerService::optimizeAllPairs " << e.what() << commit;
        throw;
    }
    catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in OptimizerService!" << commit;
        throw;
    }
}

} // end namespace server
} // end namespace fts3
