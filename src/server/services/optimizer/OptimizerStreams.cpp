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


// This part of the algorithm will check how to split the number of connections
// between the number of available transfers.
// Basically, divide the number of connections between the number of queued+active
void Optimizer::optimizeStreamsForPair(OptimizerMode optMode, const Pair &pair)
{
    // No optimization for streams, so go for 1
    if (optMode <= kOptimizerConservative) {
        dataSource->storeOptimizerStreams(pair, 1);
        return;
    }

    // Read in PairState
    auto state = currentPairStateMap[pair];

    int connectionsAvailable = state.connections;
    int availableTransfers = state.activeCount + state.queueSize;
    int streamsDecision = 1;

    if (availableTransfers > 0) {
        streamsDecision = static_cast<int>(floor(connectionsAvailable/availableTransfers));
        if (streamsDecision <= 0) {
            streamsDecision = 1;
        }
        else if (streamsDecision > maxNumberOfStreams) {
            streamsDecision = std::max(maxNumberOfStreams, 1);
        }
    }

    dataSource->storeOptimizerStreams(pair, streamsDecision);
}


}
}
