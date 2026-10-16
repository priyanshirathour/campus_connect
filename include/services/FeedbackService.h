#pragma once
#include "database/Repository.h"
#include "services/EventService.h"
#include "services/StudentService.h"
class FeedbackService {
public:
    FeedbackService(Repository& repository, StudentService& students, EventService& events);
    void submit(const std::string& studentId, const std::string& eventId, int rating, const std::string& comment);
    double averageRating(const std::string& eventId) const;
    std::vector<Feedback> forEvent(const std::string& eventId) const;
private:
    Repository& repository_; StudentService& students_; EventService& events_;
};
