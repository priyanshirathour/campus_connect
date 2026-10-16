#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// An adjacency-list graph for student-interest-club relationships.
class RecommendationGraph {
public:
    void clear();
    void addVertex(const std::string& vertex);
    void addEdge(const std::string& left, const std::string& right);
    std::vector<std::string> breadthFirstSearch(const std::string& start,
                                                std::size_t maximumResults = 100) const;
    bool hasVertex(const std::string& vertex) const;

private:
    std::unordered_map<std::string, std::unordered_set<std::string>> adjacency_;
};
