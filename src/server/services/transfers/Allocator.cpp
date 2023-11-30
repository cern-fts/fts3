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

#include "Allocator.h"
#include "MaximumFlow.h"
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"

using namespace fts3::common;
using namespace db;

namespace fts3 {
namespace server {

Allocator::AllocatorAlgorithm getAllocatorAlgorithm() {
    std::string allocatorConfig = config::ServerConfig::instance().get<std::string>("TransfersServiceAllocatorAlgorithm");
    if (allocatorConfig == "MAXIMUM_FLOW") {
        return Allocator::AllocatorAlgorithm::MAXIMUM_FLOW;
    }
    else if(allocatorConfig == "GREEDY") {
        return Allocator::AllocatorAlgorithm::GREEDY;
    }
    else {
        return Allocator::AllocatorAlgorithm::MAXIMUM_FLOW;
    }
}

Allocator::AllocatorFunction Allocator::getAllocatorFunction() {
    Allocator::AllocatorFunction function;
    switch (getAllocatorAlgorithm()) {
        case Allocator::GREEDY:
            function = &Allocator::GreedyAllocator;
            break;
        case Allocator::MAXIMUM_FLOW:
            // initializing lambda here for now
            // TODO: allow lambda to be configured 
            dummyLambda = 0; 
            function = &Allocator::MaximumFlowAllocator;
            break;
        default:
            // Use randomized algorithm as default
            function = &Allocator::GreedyAllocator;
            break;
    }
    return function;
}

void getSourceDestinationCapacities(std::map<std::string, int>& slotsLeftForSource, std::map<std::string, int>& slotsLeftForDestination, const std::vector<QueueId>& queues) {
    auto db = DBSingleton::instance().getDBObjectInstance();
    for (const auto& i : queues) {
        // To reduce queries, fill in one go limits as source and as destination
        if (slotsLeftForDestination.count(i.destSe) == 0) {
            StorageConfig seConfig = db->getStorageConfig(i.destSe);
            slotsLeftForDestination[i.destSe] = seConfig.inboundMaxActive > 0 ? seConfig.inboundMaxActive : 60;
            slotsLeftForSource[i.destSe] = seConfig.outboundMaxActive > 0 ? seConfig.outboundMaxActive : 60;
        }
        if (slotsLeftForSource.count(i.sourceSe) == 0) {
            StorageConfig seConfig = db->getStorageConfig(i.sourceSe);
            slotsLeftForDestination[i.sourceSe] = seConfig.inboundMaxActive > 0 ? seConfig.inboundMaxActive : 60;
            slotsLeftForSource[i.sourceSe] = seConfig.outboundMaxActive > 0 ? seConfig.outboundMaxActive : 60;
        }
        // Once it is filled, decrement
        slotsLeftForDestination[i.destSe] -= i.activeCount;
        slotsLeftForSource[i.sourceSe] -= i.activeCount;
    }
}

std::map<Pair, int> Allocator::GreedyAllocator(
    std::vector<QueueId> &queues
){
    auto db = DBSingleton::instance().getDBObjectInstance();
    DBSingleton::instance().getDBObjectInstance()->getQueuesWithPending(queues);

    // Retrieve link capacities
    std::map<std::string, std::list<TransferFile> > voQueues;
    std::map<Pair, int> linkCapacities = db->getLinkCapacities(queues, voQueues);
    std::map<Pair, int> allocatorMap;
    // Introduce capacities for virtual source to each source
    for (const auto& i : queues) {
        Pair vo = Pair(i.sourceSe, i.destSe);
        allocatorMap[vo] = linkCapacities[vo];
    }
    return allocatorMap;
}

std::map<Pair, int> Allocator::MaximumFlowAllocator(
    std::vector<QueueId> &queues
){
    auto db = DBSingleton::instance().getDBObjectInstance();
    DBSingleton::instance().getDBObjectInstance()->getQueuesWithPending(queues);
    std::map<std::string, int> slotsLeftForSource, slotsLeftForDestination;
    getSourceDestinationCapacities(slotsLeftForSource, slotsLeftForDestination, queues);
    
    // Retrieve link capacities
    std::map<std::string, std::list<TransferFile> > voQueues;
    std::map<Pair, int> linkCapacities = db->getLinkCapacities(queues, voQueues);
    
    // Declare graph variables
    std::map<std::string, int> seToInt;
    std::map<int, std::string> intToSe;
    int nodeCount = 0;

    MaximumFlow::MaximumFlowSolver solver;
    std::map<Pair, int> allocatorMap;
    std::map<std::pair<int, int>, int> maximumFlow;
    std::map<std::pair<int, int>, int> starvedLinks; 

    // We initialize seToInt and intToSe since our maximum flow class utilizes array indexing for optimization
    for (const auto& i : queues) {
        if (seToInt.find(i.sourceSe) == seToInt.end()) {
            seToInt[i.sourceSe] = nodeCount;
            intToSe[nodeCount] = i.sourceSe;
            nodeCount++;
        }
        if (seToInt.find(i.destSe) == seToInt.end()) {
            seToInt[i.destSe] = nodeCount;
            intToSe[nodeCount] = i.destSe;
            nodeCount++;
        }
        // determine if link is starved 
        std::pair<int, int> currLink = std::make_pair(seToInt[i.sourceSe], seToInt[i.destSe]);
        if (linkDeficitTracker.find(currLink) != linkDeficitTracker.end() && linkDeficitTracker[currLink] >= dummyLambda) {
            // keep track of which links are starved, and allocate maximum capacity (at first, to be updated below)
            starvedLinks[currLink] = linkCapacities[currLink]; 
        }
    }
    // create a vector of starved links from the map
    std::vector<std::pair<std::pair<int, int>, int>> sortedStarvedLinks(starvedLinks.begin(), starvedLinks.end()); 
    auto compareLinks = [](const std::pair<std::pair<int, int>, int>& a, std::pair<std::pair<int, int>, int>& b) {
        return a.second > b.second; 
    }
    // sort the starved links in decreasing order 
    std::sort(sortedStarvedLinks.begin(), sortedStarvedLinks.end(), compareLinks); 
    // allocate slots for the starved links 
    for (const auto& elem : sortedStarvedLinks) {
        // allocate slots to given link 
        int minCapacityStorageElements = min(slotsLeftForSource[elem.first.first], slotsLeftForDestination[elem.first.second]);
        int allocated = min(minCapacityStorageElements, elem.second);
        allocatorMap[elem.first] = allocated; 
        // update storage elements' slots 
        slotsLeftForSource[elem.first.first] -= allocated;
        slotsLeftForDestination[elem.first.second] -= allocated; 
        // update deficit tracker 
        linkDeficitTracker[elem.first] = linkCapacities[elem.first] - allocated;
    }
    // node count is incremented after each node so virtual source is nodeCount, dest is nodeCount + 1
    solver.setSource(nodeCount);
    solver.setSink(nodeCount + 1);
    solver.setNodes(nodeCount + 2);
    // Introduce capacities for virtual source to each source
    for (const auto& i : queues) {
        // if the link isn't starved, add it to the graph
        if (starvedLinks.find(std::make_pair(i.sourceSe, i.destSe)) == starvedLinks.end()) {
            solver.addEdge(nodeCount, seToInt[i.sourceSe], slotsLeftForSource[i.sourceSe]);
            solver.addEdge(seToInt[i.destSe], nodeCount + 1, slotsLeftForDestination[i.destSe]);
            solver.addEdge(seToInt[i.sourceSe], seToInt[i.destSe], linkCapacities[Pair(i.sourceSe, i.destSe)]);
        }
    }
    
    maximumFlow = solver.computeMaximumFlow();
    
    for (const auto& entry : maximumFlow) {
        allocatorMap[Pair(intToSe[entry.first.first], intToSe[entry.first.second])] = entry.second;
    }
    return allocatorMap;
}

} // end namespace server
} // end namespace fts3

