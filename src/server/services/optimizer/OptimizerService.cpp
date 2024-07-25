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
        const bool enabled = config::ServerConfig::instance().get<bool>("MonitoringMessaging");
        if (!enabled) {
            return nullptr;
        } else {
            return std::make_unique<OptimizerNotifier>(enabled,config::ServerConfig::instance().get<std::string>("MessagingDirectory"));
        }
    }
};


OptimizerService::OptimizerService(HeartBeat *beat): BaseService("OptimizerService"), beat(beat)
{
}


void OptimizerService::runService()
{
    typedef boost::posix_time::time_duration TDuration;

    auto optimizerInterval = config::ServerConfig::instance().get<TDuration>("OptimizerInterval");

    while (!boost::this_thread::interruption_requested()) {
        try {
            if (beat->isLeadNode()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer run: Start to optimize all pairs" << commit;
                optimizeAllPairs();
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer run: Finished to optimize all pairs" << commit;
            } else {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer: Not the leading node ... "<< optimizerInterval << commit;
            }
        }
        catch (std::exception &e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread OptimizerService " << e.what() <<
            fts3::common::commit;
        }
        catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread OptimizerService unknown" << fts3::common::commit;
        }
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer: Going to sleep for "<< optimizerInterval << commit;
        boost::this_thread::sleep(optimizerInterval);
    }
}

    void OptimizerService::optimizeAllPairs() {

        ThreadPool<OptimizerExecutor> execPool(10); // Run optimizer for each link in a thread pool
        auto db = db::DBSingleton::instance().getDBObjectInstance();

        // Just assume that optimizer callback can be shared across multiple threads
        OptimizerNotifier optimizerCallbacks(
                config::ServerConfig::instance().get<bool>("MonitoringMessaging"),
                config::ServerConfig::instance().get<std::string>("MessagingDirectory")
        );

        // Just read all the configurations from config file
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

            size_t pairsSize = pairs.size();

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Optimizer run: found " << pairsSize << " pairs to be optimized" << commit;

            // For each par create an optimizer task and run it in the thread pool
            for (auto pair : pairs) {

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
            int optimized = execPool.reduce(std::plus<int>());
            FTS3_COMMON_LOGGER_NEWLOG(INFO) <<"Optimizer threadPool: " << pairsSize << " pairs (" << optimized << " have been optimized)" << commit;

        }
        catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in TransfersService:optimizeAllPairs" << commit;
            execPool.interrupt();
            execPool.join();
            throw;
        }
        catch (std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService:optimizeAllPairs " << e.what() << commit;
            throw;
        }
        catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in OptimizerService!" << commit;
            throw;
        }
    }


} // end namespace server
} // end namespace fts3
