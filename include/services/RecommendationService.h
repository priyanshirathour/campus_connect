#pragma once
#include "database/Repository.h"
#include "datastructures/RecommendationGraph.h"
#include "services/ClubService.h"
#include "services/StudentService.h"
class RecommendationService {
public:
    RecommendationService(Repository& repository, StudentService& students, ClubService& clubs);
    std::vector<Club> recommendClubs(const std::string& studentId, std::size_t maximum = 5) const;
    std::vector<Event> recommendEvents(const std::string& studentId, std::size_t maximum = 5) const;
    void rebuild();
private:
    Repository& repository_; StudentService& students_; ClubService& clubs_; RecommendationGraph graph_;
};
