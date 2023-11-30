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

    
// Tracks deficit on each link; incremented as links become starved of transfers
static std::map<Pair, int> linkDeficits = std::map<Pair, int>();
static int lambda = config::ServerConfig::instance().get<int>("TransfersServiceAllocatorLambda");

Allocator::AllocatorAlgorithm getAllocatorAlgorithm() {
    std::string allocatorConfig = config::ServerConfig::instance().get<std::string>("TransfersServiceAllocatorAlgorithm");
    if (allocatorConfig == "MAXIMUM_FLOW") {
        return Allocator::AllocatorAlgorithm::MAXIMUM_FLOW;
    }
    else if(allocatorConfig == "GREEDY") {
        return Allocator::AllocatorAlgorithm::GREEDY;
    }
    else {
        return Allocator::AllocatorAlgorithm::GREEDY;
    }
}

Allocator::AllocatorFunction Allocator::getAllocatorFunction() {
    Allocator::AllocatorFunction function;
    switch (getAllocatorAlgorithm()) {
        case Allocator::GREEDY:
            function = &Allocator::GreedyAllocator;
            break;
        case Allocator::MAXIMUM_FLOW:
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
    std::map<std::string, std::list<TransferFile> > voQueues;

    // Retrieve se outbound (slotsLeftForSource) and inbound (slotsLeftForDestination) capacities
    std::map<std::string, int> slotsLeftForSource, slotsLeftForDestination;
    getSourceDestinationCapacities(slotsLeftForSource, slotsLeftForDestination, queues);
    
    // Retrieve link capacities
    std::map<Pair, int> linkCapacities = db->getLinkCapacities(queues, voQueues);

    // Return value, to be filled as allocation proceeds
    std::map<Pair, int> allocatorMap;

    /*
    *   Starvation avoidance: allocate all starved links before running max flow
    */ 

    // Map of starved links to their capacities
    std::map<Pair, int> starvedLinkCapacities; 

    for (const auto& i : queues) {
        // determine if link is starved 
        Pair currLink = Pair(i.sourceSe, i.destSe);
        if (linkDeficits.find(currLink) != linkDeficits.end() && linkDeficits[currLink] >= lambda) {
            // keep track of which links are starved, and allocate maximum capacity (at first, to be updated below)
            starvedLinkCapacities[currLink] = linkCapacities[currLink]; 
        }
    }

    // create a vector of starved links and their capacities from the map starvedLinkCapacities
    auto sortedStarvedLinks = std::vector<std::pair<Pair, int>>(starvedLinkCapacities.begin(), starvedLinkCapacities.end()); 
    // sort the starved links in decreasing order, largest deficit first
    auto compareLinks = [](const std::pair<Pair, int>& a, std::pair<Pair, int>& b) {
        return a.second > b.second; 
    };
    std::sort(sortedStarvedLinks.begin(), sortedStarvedLinks.end(), compareLinks); 

    // allocate slots for the starved links 
    for (const auto& elem : sortedStarvedLinks) {
        Pair link = elem.first;
        int linkCapacity = elem.second;

        // allocate maximum possible slots to this starved link 
        int minSeCapacity = std::min(slotsLeftForSource[link.source], slotsLeftForDestination[link.destination]);
        int numSlotsToAllocate = std::min(minSeCapacity, linkCapacity);
        allocatorMap[link] = numSlotsToAllocate; 

        // update storage elements' slots 
        slotsLeftForSource[link.source] -= numSlotsToAllocate;
        slotsLeftForDestination[link.destination] -= numSlotsToAllocate; 

        // reset this link's deficit to 0
        // note that deficit will later be incremented if link was not fully allocated (linkCapacities[link] < numSlotsToAllocate)
        linkDeficits[link] = linkCapacities[link] - numSlotsToAllocate;
    }

    /*
    *   Max flow: run maximum flow algorithm on non-starved links to maximize throughput of allocation
    */ 
    
    // Declare graph variables
    std::map<std::string, int> seToInt;
    std::map<int, std::string> intToSe;
    int nodeCount = 0;

    MaximumFlow::MaximumFlowSolver solver;
    std::map<std::pair<int, int>, int> maximumFlow;

    // Initialize seToInt and intToSe since our maximum flow class utilizes array indexing for optimization
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
    }

    // node count is incremented after each node is added so virtual source is nodeCount, dest is nodeCount + 1
    solver.setSource(nodeCount);
    solver.setSink(nodeCount + 1);
    solver.setNodes(nodeCount + 2);

    // Introduce capacities for virtual source to each source
    for (const auto& i : queues) {
        int sourceSlots = slotsLeftForSource[i.sourceSe];
        int destSlots = slotsLeftForDestination[i.destSe];
        int linkSlots = linkCapacities[Pair(i.sourceSe, i.destSe)];

        // Reduce graph size by excluding links where the source, destination, or link itself has no slots left, 
        // and starved links which have already been fully allocated
        if (sourceSlots > 0 && destSlots > 0 && linkSlots > 0) {
            solver.addEdge(nodeCount, seToInt[i.sourceSe], sourceSlots);
            solver.addEdge(seToInt[i.destSe], nodeCount + 1, destSlots);
            solver.addEdge(seToInt[i.sourceSe], seToInt[i.destSe], linkSlots);
        }
    }
    
    maximumFlow = solver.computeMaximumFlow();
    
    for (const auto& entry : maximumFlow) {
        Pair link = Pair(intToSe[entry.first.first], intToSe[entry.first.second]);
        int numSlotsToAllocate = entry.second;
        allocatorMap[link] = numSlotsToAllocate;
    }

    // for each allocated link, increment deficit by difference in capacity and allocated flow
    for (const auto& entry : allocatorMap) {
        Pair link = entry.first;
        int numSlotsToAllocate = entry.second;
        linkDeficits[link] += (linkCapacities[link] - numSlotsToAllocate);
    }

    return allocatorMap;
}

} // end namespace server
} // end namespace fts3

