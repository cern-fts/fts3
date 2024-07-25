/*
 * Copyright (c) CERN 2013-2016
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
    emaAlpha(EMA_ALPHA), pairsSize(0), pairIdx(0), pair(pair)
{
}


OptimizerExecutor::~OptimizerExecutor()
{
}


void OptimizerExecutor::setSteadyInterval(boost::posix_time::time_duration newValue)
{
    optimizerSteadyInterval = newValue;
}


void OptimizerExecutor::setMaxNumberOfStreams(int newValue)
{
    maxNumberOfStreams = newValue;
}


void OptimizerExecutor::setMaxSuccessRate(int newValue)
{
    maxSuccessRate = newValue;
}


void OptimizerExecutor::setLowSuccessRate(int newValue)
{
    lowSuccessRate = newValue;
}


void OptimizerExecutor::setBaseSuccessRate(int newValue)
{
    baseSuccessRate = newValue;
}


void OptimizerExecutor::setStepSize(int increase, int increaseAggressive, int decrease)
{
    increaseStepSize = increase;
    increaseAggressiveStepSize = increaseAggressive;
    decreaseStepSize = decrease;
}


void OptimizerExecutor::setEmaAlpha(double alpha)
{
    emaAlpha = alpha;
}


void OptimizerExecutor::run(boost::any &ctx)
{
    if (ctx.empty()) {
        ctx = 0;
    }

    int &scheduled = boost::any_cast<int &>(ctx);

    scheduled += 1;

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Optimizer run" << commit;
    try {
        runOptimizerForPair();
    }
    catch (std::exception &e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in optimizer thread: " << e.what() << commit;
    }
    catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in optimizer thread: " << commit;
    }
}


void OptimizerExecutor::runOptimizerForPair()
{
    OptimizerMode optMode = dataSource->getOptimizerMode(pair.source, pair.destination);
    if(optimizeConnectionsForPair(optMode)) {
        // Optimize streams only if optimizeConnectionsForPair did store something
        optimizeStreamsForPair(optMode);
    }
}

}
}
