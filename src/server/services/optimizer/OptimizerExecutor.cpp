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

#include "config/ServerConfig.h"
#include "OptimizerExecutor.h"
#include "OptimizerConstants.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include "db/generic/SingleDbInstance.h"

using namespace fts3::common;
using namespace fts3::config;

namespace fts3 {
namespace optimizer {


OptimizerExecutor::OptimizerExecutor(std::unique_ptr<OptimizerDataSource> ds, std::unique_ptr<OptimizerCallbacks> callbacks, const Pair& pair):
    dataSource(std::move(ds)), callbacks(std::move(callbacks)),
    optimizerSteadyInterval(boost::posix_time::seconds(60)), maxNumberOfStreams(10),
    maxSuccessRate(100), lowSuccessRate(97), baseSuccessRate(96),
    decreaseStepSize(1), increaseStepSize(1), increaseAggressiveStepSize(2),
    emaAlpha(EMA_ALPHA), pair(pair) {}


void OptimizerExecutor::run([[maybe_unused]] boost::any &ctx)
{
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Optimizer run for pair: " << pair << commit;

    try {
        runOptimizerForPair();
    } catch (const std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in Optimizer thread: " << e.what() << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in Optimizer thread!" << commit;
    }
}


void OptimizerExecutor::runOptimizerForPair()
{
    auto optMode = dataSource->getOptimizerMode(pair.source, pair.destination);

    if (optimizeConnectionsForPair(optMode)) {
        // Optimize streams only if optimizeConnectionsForPair did store something
        optimizeStreamsForPair(optMode);
    }
}

}
}
