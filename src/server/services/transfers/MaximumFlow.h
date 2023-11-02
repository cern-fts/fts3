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
#include <map>

namespace fts3
{
namespace server
{

// MaximumFlow uses Dinic's algorithm to compute the maximum flow of a network in O(V^2E)
class MaximumFlow {
private:
    struct Edge {
        int from, to; // From and to nodes;
        Edge *residual; // Each edge has a corresponding residual edge.
        // TODO: Consider type for flows and capacities
        int flow; // f(e) initialized to 0.
        const int capacity; // c(e) initialized based upon resource constraints.

        // Edge constructor
        Edge(int from, int to, int capacity);

        //  isResidual returns true if the given edges capacity is 0 else false
        bool isResidual();
        int remainingCapacity();
        // Augments the flow of an edge as well as the flow of the reverse edge
        void augment(int bottleNeck);

        // For debugging purposes. Prints the capacity and flow of an edge as well as if it is residual.
        std::string edgeToString(int s, int t);
    };

    class MaximumFlowSolver {
    public:
        static const int INF;
        int nodes;
        int source, sink;
        bool solved;
        int maximumFlow;
        std::vector<std::vector<Edge*>> graph;

        // Initializes MaximumFlowSolver with nodes, source, sink
        MaximumFlowSolver(int nodes, int source, int sink);

        // addEdge adds a forward and reverse edge to the graph. The forward edge is initialized with the specified capacity and the
        void addEdge(int from, int to, int capacity);
        std::map<std::pair<int, int>, int> computeMaximumFlow();
        int getMaximumFlow();
        void initializeEmptyFlowGraph();

    private:
        void run();
        virtual void solve() = 0;
    };

public:
    class Dinics : public MaximumFlowSolver {
    private:
        std::vector<int> level;
    public:
        Dinics();
        void addEdge(int from, int to, int capacity);
        void setSource(int s);
        void setSink(int s);
        void setNodes(int n);
    private:
        void solve() override;
        // bfs constructs the current level graph
        bool bfs();
        // dfs uses the level graph and invokes depth first search until a blocking flow is found
        int dfs(int curr, std::vector<int>& next, int flow);
    };
};
} /* namespace server */
} /* namespace fts3 */
#endif //FTS3_DINICSMAXIMUMFLOW_H
