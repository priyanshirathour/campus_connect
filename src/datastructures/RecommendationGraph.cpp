#include "datastructures/RecommendationGraph.h"

#include <queue>

void RecommendationGraph::clear() { adjacency_.clear(); }
void RecommendationGraph::addVertex(const std::string& vertex) { adjacency_[vertex]; }

void RecommendationGraph::addEdge(const std::string& left, const std::string& right) {
    if (left == right) return;
    adjacency_[left].insert(right);
    adjacency_[right].insert(left);
}

bool RecommendationGraph::hasVertex(const std::string& vertex) const {
    return adjacency_.find(vertex) != adjacency_.end();
}

std::vector<std::string> RecommendationGraph::breadthFirstSearch(
    const std::string& start, std::size_t maximumResults) const {
    std::vector<std::string> result;
    if (!hasVertex(start) || maximumResults == 0) return result;

    std::queue<std::string> pending;
    std::unordered_set<std::string> visited;
    // Mark nodes when enqueuing so cycles cannot add the same recommendation twice.
    pending.push(start);
    visited.insert(start);

    while (!pending.empty() && result.size() < maximumResults) {
        const std::string current = pending.front();
        pending.pop();
        const auto neighbours = adjacency_.find(current);
        if (neighbours == adjacency_.end()) continue;
        for (const auto& next : neighbours->second) {
            if (visited.insert(next).second) {
                result.push_back(next);
                pending.push(next);
                if (result.size() >= maximumResults) break;
            }
        }
    }
    return result;
}
