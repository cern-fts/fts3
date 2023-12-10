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

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>

#include "server/services/transfers/MaximumFlow.h"
#include <db/generic/Pair.h>
#include <db/generic/QueueId.h>

using namespace fts3::server;


BOOST_AUTO_TEST_SUITE(server)
BOOST_AUTO_TEST_SUITE(AllocatorTestSuite)



/* 
Network Graph Representation:
   [Virtual Source]
       |     |
      20    20
       |     |
   [source1] [source2]
    /    \      |
   20    20    20
  /        \    |
[dest1]   [dest2]
   |        /
   20      20
   |      /    
   [Virtual Sink]
*/

/* This method first computes N Shaped Maximum Flow. After this initial computation, 
* it allocates resources based on deficit considerations and then reruns the Maximum Flow algorithm. 
* This two-step approach ensures an efficient allocation that takes into account both current demand (via max flow) 
* and historical shortages (via deficit allocation).
*/ 
BOOST_AUTO_TEST_CASE(TestNShapedDeficitMaximumFlow)
{
    // Setup the network environment
    std::vector<QueueId> queues;
    queues.emplace_back("source1", "dest1", "voName1", 0);
    queues.emplace_back("source1", "dest2", "voName2", 0);
    queues.emplace_back("source2", "dest2", "voName3", 0);

    std::map<std::string, int> slotsLeftForSource = {{"source1", 20}, {"source2", 20}};
    std::map<std::string, int> slotsLeftForDestination = {{"dest1", 20}, {"dest2", 20}};
    std::map<Pair, int> linkCapacities = {
        {Pair("source1", "dest1"), 20},
        {Pair("source1", "dest2"), 20},
        {Pair("source2", "dest2"), 20}
    };
    // Return value, to be filled as allocation proceeds
    std::map<Pair, int> allocatorMap;

    // Initialize deficit tracker
    std::map<Pair, int> linkDeficits = std::map<Pair, int>();
    std::map<Pair, int> starvedLinkCapacities; 
    int lambda = 20;

    // Map each unique SE to an integer and vice versa
    std::map<std::string, int> seToInt;
    std::map<int, std::string> intToSe;
    int nodeCount = 0;
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
    int virtualSource = nodeCount;
    int virtualSink = nodeCount + 1;
    intToSe[virtualSink] = "virtualSink";
    intToSe[virtualSource] = "virtualSource";
    MaximumFlow<int>::MaximumFlowSolver solver = MaximumFlow<int>::MaximumFlowSolver(virtualSource, virtualSink, nodeCount + 2);
    for (const auto& i : queues) { 
        int sourceSlots = slotsLeftForSource[i.sourceSe];
        int destSlots = slotsLeftForDestination[i.destSe];
        int linkSlots = linkCapacities[Pair(i.sourceSe, i.destSe)];
        solver.addEdge(virtualSource, seToInt[i.sourceSe], sourceSlots);
        solver.addEdge(seToInt[i.destSe], virtualSink, destSlots);
        solver.addEdge(seToInt[i.sourceSe], seToInt[i.destSe], linkSlots);
    }

    // Compute the maximum flow
    std::map<std::pair<int, int>, int> maximumFlow = solver.computeMaximumFlow();
    for (const auto& entry : maximumFlow) {
        Pair link = Pair(intToSe[entry.first.first], intToSe[entry.first.second]);
        int numSlotsToAllocate = entry.second;
        allocatorMap[link] = numSlotsToAllocate;
    }
    for (const auto& src : slotsLeftForSource) {
        BOOST_CHECK(allocatorMap[Pair(intToSe[virtualSource], src.first)] <= src.second);
    }
    BOOST_CHECK_EQUAL(allocatorMap[Pair(intToSe[virtualSource], "source1")], 20);
    BOOST_CHECK_EQUAL(allocatorMap[Pair(intToSe[virtualSource], "source2")], 20);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("source1", "dest1")], 20);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("source1", "dest2")], 0);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("source2", "dest2")], 20);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("dest1", intToSe[virtualSink])], 20);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("dest2", intToSe[virtualSink])], 20);

    for (const auto& entry : allocatorMap) {
        Pair link = entry.first;
        int numSlotsToAllocate = entry.second;
        linkDeficits[link] += (linkCapacities[link] - numSlotsToAllocate);
        // log the total number of slots allocated to this activity by max flow
    }
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
    allocatorMap.clear();
    for (const auto& elem : sortedStarvedLinks) {
        Pair link = elem.first;
        int linkCapacity = elem.second;

        // allocate maximum possible slots to this starved link 
        int minSeCapacity = std::min(slotsLeftForSource[link.source], slotsLeftForDestination[link.destination]);
        int numSlotsToAllocate = std::min(minSeCapacity, linkCapacity);
        allocatorMap[Pair(link.source, link.destination)] = numSlotsToAllocate; 
        // update storage elements' slots 
        slotsLeftForSource[link.source] -= numSlotsToAllocate;
        slotsLeftForDestination[link.destination] -= numSlotsToAllocate; 
        linkDeficits[link] = linkCapacities[link] - numSlotsToAllocate;
    }

    BOOST_CHECK_EQUAL(allocatorMap[Pair("source1", "dest2")], 20);
    solver = MaximumFlow<int>::MaximumFlowSolver(virtualSink, virtualSource, nodeCount + 2);

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
    BOOST_CHECK_EQUAL(allocatorMap[Pair(intToSe[virtualSource], "source1")], 0);
    BOOST_CHECK_EQUAL(allocatorMap[Pair(intToSe[virtualSource], "source2")], 0);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("source1", "dest1")], 0);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("source1", "dest2")], 20);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("source2", "dest2")], 0);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("dest1", intToSe[virtualSink])], 0);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("dest2", intToSe[virtualSink])], 0);

}


/* Network Graph Representation:
Note we cannot represent edge crossing, treat the dest1 nodes as the same node
   [Virtual Source]
       |     |
      10    20
       |     |
   [source1] [source2]
    /  \      /  \
   10   5    20  10
  /      \  /      \
[dest1]  [dest2]  [dest1] 
   |       |       /
   15      25     /
   |       |     /
   [Virtual Sink]
*/ 
BOOST_AUTO_TEST_CASE (TestMultiWeightMaximumFlow)
{
    // Setup the network environment
    std::vector<QueueId> queues;
    queues.emplace_back("source1", "dest1", "voName1", 0);
    queues.emplace_back("source1", "dest2", "voName2", 0);
    queues.emplace_back("source2", "dest1", "voName3", 0);
    queues.emplace_back("source2", "dest2", "voName4", 0);

    // Setup the network environment
    std::map<std::string, int> slotsLeftForSource = {{"source1", 10}, {"source2", 20}};
    std::map<std::string, int> slotsLeftForDestination = {{"dest1", 15}, {"dest2", 25}};
    std::map<Pair, int> linkCapacities = {
        {Pair("source1", "dest1"), 10},
        {Pair("source1", "dest2"), 5},
        {Pair("source2", "dest1"), 20},
        {Pair("source2", "dest2"), 10}
    };
    // Return value, to be filled as allocation proceeds
    std::map<Pair, int> allocatorMap;

     // Map each unique SE to an integer and vice versa
    std::map<std::string, int> seToInt;
    std::map<int, std::string> intToSe;
    int nodeCount = 0;
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
    int virtualSource = nodeCount;
    int virtualSink = nodeCount + 1;
    intToSe[virtualSink] = "virtualSink";
    intToSe[virtualSource] = "virtualSource";
    MaximumFlow<int>::MaximumFlowSolver solver = MaximumFlow<int>::MaximumFlowSolver(virtualSource, virtualSink, nodeCount + 2);

    
    for (const auto& i : queues) { 
        int sourceSlots = slotsLeftForSource[i.sourceSe];
        int destSlots = slotsLeftForDestination[i.destSe];
        int linkSlots = linkCapacities[Pair(i.sourceSe, i.destSe)];

        solver.addEdge(virtualSource, seToInt[i.sourceSe], sourceSlots);
        solver.addEdge(seToInt[i.destSe], virtualSink, destSlots);
        solver.addEdge(seToInt[i.sourceSe], seToInt[i.destSe], linkSlots);
    }
    // Compute the maximum flow
    std::map<std::pair<int, int>, int> maximumFlow = solver.computeMaximumFlow();
    maximumFlow = solver.computeMaximumFlow();

    for (const auto& entry : maximumFlow) {
        Pair link = Pair(intToSe[entry.first.first], intToSe[entry.first.second]);
        int numSlotsToAllocate = entry.second;
        allocatorMap[link] = numSlotsToAllocate;
    }

    BOOST_CHECK_EQUAL(allocatorMap[Pair(intToSe[virtualSource], "source1")], 10);
    BOOST_CHECK_EQUAL(allocatorMap[Pair(intToSe[virtualSource], "source2")], 20);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("source1", "dest1")], 5);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("source1", "dest2")], 5);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("source2", "dest1")], 10);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("source2", "dest2")], 10);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("dest1", intToSe[virtualSink])], 15);
    BOOST_CHECK_EQUAL(allocatorMap[Pair("dest2", intToSe[virtualSink])], 15);
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
