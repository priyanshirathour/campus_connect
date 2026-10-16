#pragma once
#include "database/Repository.h"
#include "services/AttendanceService.h"
#include "services/EventService.h"
#include "services/StudentService.h"

// Issues one certificate per student/event pair, after attendance is marked present.
class CertificateService {
public:
    CertificateService(Repository& repository, StudentService& students, EventService& events, AttendanceService& attendance);
    Certificate issue(const std::string& studentId, const std::string& eventId);
    std::vector<Certificate> forStudent(const std::string& studentId) const;
private:
    Repository& repository_; StudentService& students_; EventService& events_; AttendanceService& attendance_;
};
