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

#ifndef FTS3_DINICSMAXIMUMFLOW_H
#define FTS3_DINICSMAXIMUMFLOW_H

#include <iostream>
#include <vector>
#include <queue>

// DinicsMaximumFlow computes the maximum flow of a network in O(V^2E)
class DinicsMaximumFlow {
private:
    struct Edge {
        int from, to; // From and to capacities.
        Edge *residual; // Each edge has a corresponding residual edge.
        // TODO: Consider type for flows and capacities
        long long flow; // f(e) initialized to 0.
        const long long capacity; // c(e) initialized based upon resource constraints.

        // Edge constructor
        Edge(int from, int to, long long capacity);

        //  isResidual returns true if the given edges capacity is 0 else false
        bool isResidual();
        long long remainingCapacity();
        // Augments the flow of an edge as well as the flow of the reverse edge
        void augment(long long bottleNeck);

        // For debugging purposes. Prints the capacity and flow of an edge as well as if it is residual.
        std::string edgeToString(int s, int t);
    };

    class MaximumFlowSolver {
    public:
        // TODO: Consider the initialization of infinity
        // We initialize INF as LLONG_MAX / 2 to avoid overflow
        static const long long INF;
        int nodes, source, sink;
        bool solved;
        long long maximumFlow;
        std::vector<std::vector<Edge*>> graph;

        // Initializes MaximumFlowSolver with nodes, source, sink
        MaximumFlowSolver(int nodes, int source, int sink);

        // addEdge adds a forward and reverse edge to the graph. The forward edge is initialized with the specified capacity and the
        void addEdge(int from, int to, long long capacity);
        std::vector<std::vector<Edge*>> retrieveGraph();
        long long retrieveMaximumFlow();

    private:
        // initializeEmptyFlowGraph creates an empty graph with n nodes
        void initializeEmptyFlowGraph();
        // run solves the graph
        void run();
        // computes maximum flow on the graph
        virtual void solve() = 0;
    };

    class Dinics : public MaximumFlowSolver {
    private:
        std::vector<int> level;

    public:
        Dinics(int nodes, int source, int sink);

    private:
        void solve() override;
        // bfs constructs the current level graph
        bool bfs();
        // dfs uses the level graph and invokes depth first search until a blocking flow is found
        long long dfs(int curr, std::vector<int>& next, long long flow);
    };
};
#endif //FTS3_DINICSMAXIMUMFLOW_H
