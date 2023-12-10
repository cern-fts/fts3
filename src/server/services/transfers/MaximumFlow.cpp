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
template<typename T>
MaximumFlow<T>::Edge::Edge(int from, int to, T capacity)
        : from(from), to(to), residual(nullptr), flow(0), capacity(capacity) {}

template<typename T>
bool MaximumFlow<T>::Edge::isResidual() {
    return capacity == 0;
}

template<typename T>
T MaximumFlow<T>::Edge::remainingCapacity() {
    return capacity - flow;
}

template<typename T>
void MaximumFlow<T>::Edge::augment(T bottleNeck) {
    flow += bottleNeck;
    residual->flow -= bottleNeck;
}

template<typename T>
std::string MaximumFlow<T>::Edge::edgeToString(int s, int t) {
    std::string u = (from == s) ? "s" : ((from == t) ? "t" : std::to_string(from));
    std::string v = (to == s) ? "s" : ((to == t) ? "t" : std::to_string(to));
    return "Edge " + u + " -> " + v + " | flow = " + std::to_string(flow) +
           " | capacity = " + std::to_string(capacity) + " | is residual: " + (isResidual() ? "true" : "false");
}

template<typename T>
const int MaximumFlow<T>::MaximumFlowSolver::INF = (unsigned)!((int)0);

template<typename T>
MaximumFlow<T>::MaximumFlowSolver::MaximumFlowSolver(int source, int sink, int nodes)
        : nodes(nodes), source(source), sink(sink), solved(false), maximumFlow(0) {
    level = std::vector<int>(nodes, 0); // Initialize level vector with size n
    initializeEmptyFlowGraph(); // Initialize the flow graph
}

template<typename T>
void MaximumFlow<T>::MaximumFlowSolver::addEdge(int from, int to, T capacity) {
    
    
    if (capacity <= 0) {
            throw std::invalid_argument("Forward edge capacity <= 0");
        }

        // Check if the edge already exists
        auto fromMapIt = graph.find(from);
        if (fromMapIt != graph.end() && fromMapIt->second.find(to) != fromMapIt->second.end()) {
            // Edge from 'from' to 'to' already exists
            return;
        }

        // Create new edge and its residual
        Edge* e1 = new Edge(from, to, capacity);
        Edge* e2 = new Edge(to, from, 0); // Residual edge
        e1->residual = e2;
        e2->residual = e1;

        // Add the edge and its residual to the graph
        graph[from][to] = e1;
        graph[to][from] = e2;
}

template<typename T>
std::map<std::pair<int, int>, T> MaximumFlow<T>::MaximumFlowSolver::computeMaximumFlow() {
    run();
    std::map<std::pair<int, int>, T> flowMap;

    // Iterate over each node and its corresponding edges
    for (const auto& fromPair : graph) {
        int fromNode = fromPair.first;
        const auto& edges = fromPair.second;

        for (const auto& toPair : edges) {
            int toNode = toPair.first;
            const Edge* edge = toPair.second;

            // Only return forward flows
            if (edge->flow >= 0) {
                flowMap[std::make_pair(fromNode, toNode)] = edge->flow;
            }
        }
    }

    return flowMap;
}

template<typename T>
T MaximumFlow<T>::MaximumFlowSolver::getMaximumFlow() {
    run();
    return maximumFlow;
}

template<typename T>
void MaximumFlow<T>::MaximumFlowSolver::initializeEmptyFlowGraph() {
    graph.clear();
}

template<typename T>
void MaximumFlow<T>::MaximumFlowSolver::run() {
    if (solved) return;
    std::vector<int> next(nodes);
        while (bfs()) {
            std::fill(next.begin(), next.end(), 0);
        while (true) {
            T flow = dfs(source, next, INF);
            if (flow == 0) {
                break;
            }
            maximumFlow += flow;
        }
    }
    solved = true;
}

template<typename T>
bool MaximumFlow<T>::MaximumFlowSolver::bfs() {
    // Initialize all levels to -1
    std::fill(level.begin(), level.end(), -1);
    std::queue<int> queue;
    queue.push(source);
    level[source] = 0;

    while (!queue.empty()) {
        int node = queue.front();
        queue.pop();

        // Check if the node is in the graph
        auto nodeIt = graph.find(node);
        if (nodeIt != graph.end()) {
            // Iterate over edges of the current node
            for (const auto& edgePair : nodeIt->second) {
                Edge* edge = edgePair.second;
                T edgeCapacity = edge->remainingCapacity();

                // Add node to next level of level graph if it has positive capacity, and it brings us closer to the sink
                if (edgeCapacity > 0 && level[edge->to] == -1) {
                    level[edge->to] = level[node] + 1;
                    queue.push(edge->to);
                }
            }
        }
    }

    return level[sink] != -1;
}

template<typename T>
T MaximumFlow<T>::MaximumFlowSolver::dfs(int curr, std::vector<int>& next, T flow) {
    if (curr == sink) return flow;

    // Since we can't directly use the 'next' index for a map, we use an iterator
    auto& edges = graph[curr];
    int numEdges = edges.size();
    auto edgeIt = edges.begin();
    std::advance(edgeIt, next[curr]);  // Advance the iterator to the current position

    for (; next[curr] < numEdges; next[curr]++, edgeIt++) {
        Edge* edge = edgeIt->second;
        T edgeCapacity = edge->remainingCapacity();

        if (edgeCapacity > 0 && level[edge->to] == level[curr] + 1) {
            T bottleNeck = dfs(edge->to, next, std::min(flow, edgeCapacity));
            if (bottleNeck > 0) {
                edge->augment(bottleNeck);
                return bottleNeck;
            }
        }
    }
    // blocking flow found
    return 0;
}

template class fts3::server::MaximumFlow<int>;
} /* namespace cli */
} /* namespace fts3 */

