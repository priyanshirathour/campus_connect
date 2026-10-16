#include "services/AttendanceService.h"

#include "exceptions/CampusConnectException.h"
#include "utils/Utils.h"

AttendanceService::AttendanceService(Repository& repository, StudentService& students,
                                     EventService& events, RegistrationService& registrations)
    : repository_(repository), students_(students), events_(events), registrations_(registrations) {
    refresh();
}

void AttendanceService::refresh() {
    attendanceById_.clear();
    for (const auto& attendance : repository_.allAttendance()) {
        attendanceById_[attendance.id] = attendance;
    }
}

void AttendanceService::mark(const std::string& studentId, const std::string& eventId, bool present) {
    students_.get(studentId);
    events_.get(eventId);
    if (!registrations_.hasActiveRegistration(studentId, eventId)) {
        throw CampusConnectException("Attendance requires an active registration for this event.");
    }

    Attendance attendance{"A-" + studentId + "-" + eventId,
                          studentId, eventId, present, nowTimestamp()};
    repository_.save(attendance);
    attendanceById_[attendance.id] = attendance;
}

std::vector<Attendance> AttendanceService::forEvent(const std::string& eventId) const {
    std::vector<Attendance> attendanceForEvent;
    for (const auto& entry : attendanceById_) {
        if (entry.second.eventId == eventId) {
            attendanceForEvent.push_back(entry.second);
        }
    }
    return attendanceForEvent;
}

bool AttendanceService::isPresent(const std::string& studentId, const std::string& eventId) const {
    const std::string attendanceId = "A-" + studentId + "-" + eventId;
    const auto attendance = attendanceById_.find(attendanceId);
    return attendance != attendanceById_.end() && attendance->second.present;
}
