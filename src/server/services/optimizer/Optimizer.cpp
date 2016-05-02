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

#include "Optimizer.h"
#include "OptimizerConstants.h"
#include "common/Exceptions.h"
#include "common/Logger.h"

using namespace fts3::common;


namespace fts3 {
namespace optimizer {


Optimizer::Optimizer(OptimizerDataSource *ds):
    dataSource(ds), optimizerSteadyInterval(300),
    globalMaxPerLink(DEFAULT_MAX_ACTIVE_PER_LINK),
    globalMaxPerStorage(DEFAULT_MAX_ACTIVE_ENDPOINT_LINK),
    optimizerMode(1)
{
}


Optimizer::~Optimizer()
{
}


void Optimizer::setSteadyInterval(int newValue)
{
    optimizerSteadyInterval = newValue;
}


void Optimizer::run(void)
{
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Optimizer run" << commit;

    // Query global config once
    globalMaxPerStorage = dataSource->getGlobalStorageLimit();
    if (globalMaxPerStorage <= 0) {
        globalMaxPerStorage = DEFAULT_MAX_ACTIVE_ENDPOINT_LINK;
    }

    globalMaxPerLink = dataSource->getGlobalLinkLimit();
    if (globalMaxPerLink <= 0) {
        globalMaxPerLink = DEFAULT_MAX_ACTIVE_PER_LINK;
    }

    optimizerMode = dataSource->getOptimizerMode();
    if (optimizerMode <= 0) {
        optimizerMode = 1;
    }

    try {
        std::list<Pair> pairs = dataSource->getActivePairs();

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
    optimizeConnectionsForPair(pair);
    optimizeStreamsForPair(pair);
}

}
}
