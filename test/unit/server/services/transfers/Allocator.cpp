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

using namespace fts3::server;

BOOST_AUTO_TEST_SUITE(server)
BOOST_AUTO_TEST_SUITE(AllocatorTestSuite)

BOOST_AUTO_TEST_CASE(TrivialMaximumFlow)
{
    // Setup the network scenario
    std::map<std::string, int> slotsLeftForSource = {{"source1", 20}, {"source2", 20}};
    std::map<std::string, int> slotsLeftForDestination = {{"dest1", 20}, {"dest2", 20}};
    std::map<std::pair<std::string, std::string>, int> linkCapacities = {
        {std::make_pair("source1", "dest1"), 20},
        {std::make_pair("source1", "dest2"), 20},
        {std::make_pair("source2", "dest2"), 20}
    };
    // Initialize the DinicsMaximumFlow solver
    MaximumFlow::Dinics solver;

    // Map each unique SE to an integer
    std::map<std::string, int> seToInt = {{"source1", 0}, {"source2", 1}, {"dest1", 2}, {"dest2", 3}};
    int virtualSource = 4;
    int virtualSink = 5;
    solver.setSource(virtualSource);
    solver.setSink(virtualSink);
    solver.setNodes(6); // Total Nodes (4 real + 2 virtual)

    // Add edges from virtual source to real sources
    for (const auto& src : slotsLeftForSource) {
        solver.addEdge(virtualSource, seToInt[src.first], src.second);
    }

    // Add edges from real destinations to virtual sink
    for (const auto& dest : slotsLeftForDestination) {
        solver.addEdge(seToInt[dest.first], virtualSink, dest.second);
    }

    // Add edges between sources and destinations based on link capacities
    for (const auto& link : linkCapacities) {
        solver.addEdge(seToInt[link.first.first], seToInt[link.first.second], link.second);
    }

    // Compute the maximum flow

    std::map<std::pair<int, int>, int> maximumFlow = solver.computeMaximumFlow();

    // For example, check that the flow from virtual source to a real source does not exceed its capacity
    for (const auto& src : slotsLeftForSource) {
        BOOST_CHECK(maximumFlow[std::make_pair(virtualSource, seToInt[src.first])] <= src.second);
    }
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(virtualSource, seToInt["source1"])], 20);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(virtualSource, seToInt["source2"])], 20);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(seToInt["source1"], seToInt["dest1"])], 20);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(seToInt["source1"], seToInt["dest2"])], 0);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(seToInt["source2"], seToInt["dest2"])], 20);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(seToInt["dest1"], virtualSink)], 20);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(seToInt["dest2"], virtualSink)], 20);
}


BOOST_AUTO_TEST_CASE (TestLessTrivialMaximumFlow)
{
    // Setup the network scenario
    std::map<std::string, int> slotsLeftForSource = {{"source1", 10}, {"source2", 20}};
    std::map<std::string, int> slotsLeftForDestination = {{"dest1", 15}, {"dest2", 25}};
    std::map<std::pair<std::string, std::string>, int> linkCapacities = {
        {std::make_pair("source1", "dest1"), 10},
        {std::make_pair("source1", "dest2"), 5},
        {std::make_pair("source2", "dest1"), 20},
        {std::make_pair("source2", "dest2"), 10}
    };
    // Initialize the DinicsMaximumFlow solver
    MaximumFlow::Dinics solver;

    // Map each unique SE to an integer
    std::map<std::string, int> seToInt = {{"source1", 0}, {"source2", 1}, {"dest1", 2}, {"dest2", 3}};
    int virtualSource = 4;
    int virtualSink = 5;
    solver.setSource(virtualSource);
    solver.setSink(virtualSink);
    solver.setNodes(6); // Total Nodes (4 real + 2 virtual)

    // Add edges from virtual source to real sources
    for (const auto& src : slotsLeftForSource) {
        solver.addEdge(virtualSource, seToInt[src.first], src.second);
    }

    // Add edges from real destinations to virtual sink
    for (const auto& dest : slotsLeftForDestination) {
        solver.addEdge(seToInt[dest.first], virtualSink, dest.second);
    }

    // Add edges between sources and destinations based on link capacities
    for (const auto& link : linkCapacities) {
        solver.addEdge(seToInt[link.first.first], seToInt[link.first.second], link.second);
    }

    // Compute the maximum flow

    std::map<std::pair<int, int>, int> maximumFlow = solver.computeMaximumFlow();

    // For example, check that the flow from virtual source to a real source does not exceed its capacity
    for (const auto& src : slotsLeftForSource) {
        BOOST_CHECK(maximumFlow[std::make_pair(virtualSource, seToInt[src.first])] <= src.second);
    }
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(virtualSource, seToInt["source1"])], 10);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(virtualSource, seToInt["source2"])], 20);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(seToInt["source1"], seToInt["dest1"])], 5);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(seToInt["source1"], seToInt["dest2"])], 5);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(seToInt["source2"], seToInt["dest1"])], 10);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(seToInt["source2"], seToInt["dest2"])], 10);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(seToInt["dest1"], virtualSink)], 15);
    BOOST_CHECK_EQUAL(maximumFlow[std::make_pair(seToInt["dest2"], virtualSink)], 15);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
