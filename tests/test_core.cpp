#include "datastructures/RecommendationGraph.h"
#include "datastructures/WaitQueue.h"
#include "utils/Utils.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main() {
    // FIFO and duplicate protection.
    WaitQueue queue;
    queue.push("S1"); queue.push("S2"); queue.push("S1");
    assert(queue.size() == 2);
    assert(queue.contains("S2"));
    assert(queue.pop() == "S1");
    assert(queue.pop() == "S2");
    assert(queue.empty());

    // Removing a cancelled waiter must preserve the relative order of everyone else.
    queue.load({"S1", "S2", "S3"});
    assert(queue.remove("S2"));
    assert((queue.toVector() == std::vector<std::string>{"S1", "S3"}));
    assert(queue.pop() == "S1");
    assert(queue.pop() == "S3");

    // Adjacency-list edges are traversed breadth-first.
    RecommendationGraph graph;
    graph.addEdge("student:S1", "interest:cpp");
    graph.addEdge("interest:cpp", "club:C1");
    graph.addEdge("club:C1", "student:S2");
    const auto reachable = graph.breadthFirstSearch("student:S1", 10);
    assert(reachable.size() == 3);
    assert(graph.hasVertex("club:C1"));
    assert(graph.breadthFirstSearch("missing", 10).empty());

    std::cout << "All Campus Connect core data-structure tests passed.\n";
    return 0;
}
