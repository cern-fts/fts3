/*
 * Copyright (c) CERN 2017
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

#include "VoShares.h"
#include "db/generic/SingleDbInstance.h"
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/discrete_distribution.hpp>
#include <common/Logger.h>

using namespace db;
using namespace fts3::common;

boost::mt19937 generator;

namespace fts3 {
namespace server {


/**
 * Given the pair, a list of vos for the pair, and a list of weights for VOs, pick one
 * based on those weights.
 * @note If a VO is not on the map, it will fallback to 'public', if it is there
 * @note If the weight for a VO/public is 0, it will never be picked!
 */
static QueueId selectQueueForPair(const Pair &pair,
    const std::vector<std::string> &vos, const std::map<std::string, double> &weights)
{
    // Weights per position in vos vector
    std::vector<double> finalWeights(vos.size());

    // Get the public (catchall weight)
    // If there is no config, this is the only weight!
    double publicWeight = 0;
    if (weights.empty()) {
        publicWeight = 1;
    }
    else {
        auto publicIter = weights.find("public");
        if (publicIter != weights.end()) {
            publicWeight = publicIter->second;
        }
    }

    // Need to calculate how many "public" there are, so we can split
    int publicCount = 0;
    for (auto i = vos.begin(); i != vos.end(); ++i) {
        if (weights.find(*i) == weights.end()) {
            ++publicCount;
        }
    }
    publicWeight /= static_cast<double>(publicCount);

    // Second pass, fill up the weights
    int pos = 0;
    for (auto i = vos.begin(); i != vos.end(); ++i, ++pos) {
        auto wIter = weights.find(*i);
        if (wIter == weights.end()) {
            finalWeights[pos] = publicWeight;
        }
        else {
            finalWeights[pos] = wIter->second;
        }
    }

    // And pick one at random
    boost::random::discrete_distribution<> dist(finalWeights);
    int chosen = dist(generator);
    return QueueId(pair.source, pair.destination, vos[chosen]);
}


std::vector<QueueId> applyVoShares(const std::vector<QueueId> queues)
{
    // Vo list for each pair
    std::map<Pair, std::vector<std::string>> vosPerPair;
    for (auto i = queues.begin(); i != queues.end(); ++i) {
        vosPerPair[Pair(i->sourceSe, i->destSe)].push_back(i->voName);
    }

    // One VO per pair
    std::vector<QueueId> result;
    for (auto j = vosPerPair.begin(); j != vosPerPair.end(); ++j) {
        const Pair &p = j->first;
        const std::vector<std::string> &vos = j->second;
        std::vector<ShareConfig> shares = DBSingleton::instance().getDBObjectInstance()->getShareConfig(p.source, p.destination);

        std::map<std::string, double> weights;
        for (auto k = shares.begin(); k != shares.end(); ++k) {
            weights[k->vo] = k->weight;
        }

        QueueId chosen = selectQueueForPair(p, vos, weights);

        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Chosen " << chosen.voName << " for " << p << commit;
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Options were " << commit;
        for (auto i = vos.begin(); i != vos.end(); ++i) {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "\t" << *i << commit;
        }

        result.emplace_back(chosen);
    }
    return result;
}


}
}