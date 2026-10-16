#pragma once
#include "database/Repository.h"
#include "services/EventService.h"
#include "services/RegistrationService.h"
#include "services/StudentService.h"
#include <unordered_map>

// Records attendance for active registrations and provides the certificate eligibility check.
class AttendanceService {
public:
    AttendanceService(Repository& repository, StudentService& students,
                      EventService& events, RegistrationService& registrations);
    void mark(const std::string& studentId, const std::string& eventId, bool present);
    std::vector<Attendance> forEvent(const std::string& eventId) const;
    bool isPresent(const std::string& studentId, const std::string& eventId) const;
private:
    Repository& repository_; StudentService& students_; EventService& events_; RegistrationService& registrations_;
    std::unordered_map<std::string, Attendance> attendanceById_;
    void refresh();
};
