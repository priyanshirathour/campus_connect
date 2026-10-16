#pragma once
#include "database/Repository.h"
#include "services/ClubService.h"
#include <unordered_map>

// Owns event validation, lookup, date ordering, and search behavior.
class EventService {
public:
    EventService(Repository& repository, ClubService& clubs);
    const Event& get(const std::string& id) const;
    std::vector<Event> all() const;
    std::vector<Event> sortedByDate() const;
    std::vector<Event> search(const std::string& query) const;
    void add(const Event& event);
    void update(const Event& event);
private:
    Repository& repository_;
    ClubService& clubs_;
    std::unordered_map<std::string, Event> eventsById_;
    void refresh();
};
