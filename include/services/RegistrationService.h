#pragma once
#include "database/Repository.h"
#include "datastructures/WaitQueue.h"
#include "services/EventService.h"
#include "services/NotificationService.h"
#include "services/StudentService.h"
#include <unordered_map>

// Enforces registration rules and preserves FIFO order when rebuilding or promoting waitlists.
class RegistrationService {
public:
    RegistrationService(Repository& repository, StudentService& students,
                        EventService& events, NotificationService& notifications);
    RegistrationStatus registerStudent(const std::string& studentId, const std::string& eventId);
    void cancel(const std::string& studentId, const std::string& eventId);
    std::vector<Registration> forStudent(const std::string& studentId) const;
    std::vector<std::string> waitlistForEvent(const std::string& eventId) const;
    bool hasActiveRegistration(const std::string& studentId, const std::string& eventId) const;
    int confirmedCount(const std::string& eventId) const;
private:
    Repository& repository_;
    StudentService& students_;
    EventService& events_;
    NotificationService& notifications_;
    std::unordered_map<std::string, Registration> registrationsById_;
    std::unordered_map<std::string, WaitQueue> waitlistsByEvent_;
    void refresh();
    Registration* findActive(const std::string& studentId, const std::string& eventId);
};
