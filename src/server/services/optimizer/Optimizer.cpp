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
#include "Optimizer.h"
#include "OptimizerConstants.h"
#include "common/Exceptions.h"
#include "common/Logger.h"

using namespace fts3::common;
using namespace fts3::config;

namespace fts3 {
namespace optimizer {


Optimizer::Optimizer(OptimizerDataSource *ds, OptimizerCallbacks *callbacks):
    dataSource(ds), callbacks(callbacks),
    optimizerSteadyInterval(boost::posix_time::seconds(60)), maxNumberOfStreams(10),
    maxSuccessRate(100), lowSuccessRate(97), baseSuccessRate(96),
    decreaseStepSize(1), increaseStepSize(1), increaseAggressiveStepSize(2),
    emaAlpha(EMA_ALPHA)
{
}


Optimizer::~Optimizer()
{
}


void Optimizer::setSteadyInterval(boost::posix_time::time_duration newValue)
{
    optimizerSteadyInterval = newValue;
}


void Optimizer::setMaxNumberOfStreams(int newValue)
{
    maxNumberOfStreams = newValue;
}


void Optimizer::setMaxSuccessRate(int newValue)
{
    maxSuccessRate = newValue;
}


void Optimizer::setLowSuccessRate(int newValue)
{
    lowSuccessRate = newValue;
}


void Optimizer::setBaseSuccessRate(int newValue)
{
    baseSuccessRate = newValue;
}


void Optimizer::setStepSize(int increase, int increaseAggressive, int decrease)
{
    increaseStepSize = increase;
    increaseAggressiveStepSize = increaseAggressive;
    decreaseStepSize = decrease;
}



void Optimizer::setEmaAlpha(double alpha)
{
    emaAlpha = alpha;
}

// void Optimizer::updateOptimizerInputState(const std::list<Pair> &pairs) {


// }

// void Optimizer::updateOptimizerDecisions(const std::list<Pair> &pairs) {

// }

void Optimizer::run(void)
{
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Optimizer run" << commit;
    try {
        std::list<Pair> pairs = dataSource->getActivePairs();
        // Make sure the order is always the same
        // See FTS-1094
        pairs.sort();

        // Compute necessary statistics
        for (auto i = pairs.begin(); i != pairs.end(); ++i) {
            Optimizer::getPairState(*i);
        }

        // Compute global statistics that will be used for every pair
        std::list<std::string> storages = dataSource->getActiveStorages();
        for (auto i = storages.begin(); i != storages.end(); ++i) {
            Optimizer::getStorageState(*i);
        }
        

        // Use statistics to optimize
        for (auto i = pairs.begin(); i != pairs.end(); ++i) {
            runOptimizerForPair(*i);
        }

    }
    catch (std::exception &e) {
        throw SystemError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        throw SystemError(std::string(__func__) + ": Caught exception ");
    }
}


void Optimizer::runOptimizerForPair(const Pair &pair)
{
    OptimizerMode optMode = dataSource->getOptimizerMode(pair.source, pair.destination);
    if(optimizeConnectionsForPair(optMode, pair)) {
        // Optimize streams only if optimizeConnectionsForPair did store something
        optimizeStreamsForPair(optMode, pair);
    }
}

}
}
