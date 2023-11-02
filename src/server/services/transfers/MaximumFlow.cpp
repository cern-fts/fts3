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
#include "MaximumFlow.h"

namespace fts3
{
namespace server
{
MaximumFlow::Edge::Edge(int from, int to, int capacity)
        : from(from), to(to), residual(nullptr), flow(0), capacity(capacity) {}

bool MaximumFlow::Edge::isResidual() {
    return capacity == 0;
}

int MaximumFlow::Edge::remainingCapacity() {
    return capacity - flow;
}

void MaximumFlow::Edge::augment(int bottleNeck) {
    flow += bottleNeck;
    residual->flow -= bottleNeck;
}

std::string MaximumFlow::Edge::edgeToString(int s, int t) {
    std::string u = (from == s) ? "s" : ((from == t) ? "t" : std::to_string(from));
    std::string v = (to == s) ? "s" : ((to == t) ? "t" : std::to_string(to));
    return "Edge " + u + " -> " + v + " | flow = " + std::to_string(flow) +
           " | capacity = " + std::to_string(capacity) + " | is residual: " + (isResidual() ? "true" : "false");
}

const int MaximumFlow::MaximumFlowSolver::INF = (unsigned)!((int)0);

MaximumFlow::MaximumFlowSolver::MaximumFlowSolver()
        : nodes(0), source(0), sink(0), solved(false), maximumFlow(0) {
    initializeEmptyFlowGraph();
}

void MaximumFlow::MaximumFlowSolver::addEdge(int from, int to, int capacity) {
    if (capacity <= 0) {
        throw std::invalid_argument("Forward edge capacity <= 0");
    }
    // From edge
    Edge* e1 = new Edge(from, to, capacity);
    // To Edge
    Edge* e2 = new Edge(to, from, 0);
    e1->residual = e2;
    e2->residual = e1;

    graph[from].push_back(e1);
    graph[to].push_back(e2);
}

std::map<std::pair<int, int>, int> MaximumFlow::MaximumFlowSolver::computeMaximumFlow() {
    run();
    std::map<std::pair<int, int>, int> flowMap;
    for (const auto& edgeList : graph) {
        for (const Edge* edge : edgeList) {
            // Only return forward flows
            if (edge->flow >= 0) {
                flowMap[std::make_pair(edge->from, edge->to)] = edge->flow;
            }
        }
    }
    return flowMap;
}

int MaximumFlow::MaximumFlowSolver::getMaximumFlow() {
    run();
    return maximumFlow;
}

void MaximumFlow::MaximumFlowSolver::initializeEmptyFlowGraph() {
    graph = std::vector<std::vector<MaximumFlow::Edge*>>(nodes);
}

void MaximumFlow::MaximumFlowSolver::run() {
    if (solved) return;
    std::vector<int> next(nodes);
        while (bfs()) {
            std::fill(next.begin(), next.end(), 0);
        while (true) {
            int flow = dfs(source, next, INF);
            if (flow == 0) {
                break;
            }
            maximumFlow += flow;
        }
    }
    solved = true;
}

// Setters implementation
void MaximumFlow::MaximumFlowSolver::setSource(int s) {
    source = s;
}

void MaximumFlow::MaximumFlowSolver::setSink(int s) {
    sink = s;
}

void MaximumFlow::MaximumFlowSolver::setNodes(int n) {
    nodes = n;
    level = std::vector<int>(n, 0); // Initialize level vector with size n
    initializeEmptyFlowGraph(); // Initialize the flow graph
}

bool MaximumFlow::MaximumFlowSolver::bfs() {
    // Initialize all levels to -1
    std::fill(level.begin(), level.end(), -1);
    std::queue<int> queue;
    queue.push(source);
    level[source] = 0;
    while (!queue.empty()) {
        int node = queue.front();
        queue.pop();
        for (Edge* edge : graph[node]) {
            int edgeCapacity = edge->remainingCapacity();
            // Add node to next level of level graph if it has positive capacity, and it brings us closer to the sink
            if (edgeCapacity > 0 && level[edge->to] == -1) {
                level[edge->to] = level[node] + 1;
                queue.push(edge->to);
            }
        }
    }
    return level[sink] != -1;
}

int  MaximumFlow::MaximumFlowSolver::dfs(int curr, std::vector<int>& next, int flow) {
    if (curr == sink) return flow;
    int numEdges = graph[curr].size();
    for (; next[curr] < numEdges; next[curr]++) {
        Edge* edge = graph[curr][next[curr]];
        int edgeCapacity = edge->remainingCapacity();
        if (edgeCapacity > 0 && level[edge->to] == level[curr] + 1) {
            int bottleNeck = dfs(edge->to, next, std::min(flow, edgeCapacity));
            if (bottleNeck > 0) {
                edge->augment(bottleNeck);
                return bottleNeck;
            }
        }
    }
    // blocking flow found
    return 0;
}
} /* namespace cli */
} /* namespace fts3 */

