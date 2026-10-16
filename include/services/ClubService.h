#pragma once
#include "database/Repository.h"
#include "services/StudentService.h"
#include <unordered_map>

// Keeps the in-memory club index and validates membership before persistence.
class ClubService {
public:
    ClubService(Repository& repository, StudentService& students);
    const Club& get(const std::string& id) const;
    std::vector<Club> all() const;
    void add(const Club& club);
    void addMember(const std::string& clubId, const std::string& studentId);
private:
    Repository& repository_;
    StudentService& students_;
    std::unordered_map<std::string, Club> clubsById_;
    void refresh();
};
