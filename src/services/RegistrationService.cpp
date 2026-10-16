#include "services/RegistrationService.h"
#include "exceptions/CampusConnectException.h"
#include "utils/Utils.h"

#include <algorithm>

RegistrationService::RegistrationService(Repository& repository, StudentService& students,
                                         EventService& events, NotificationService& notifications)
    : repository_(repository), students_(students), events_(events), notifications_(notifications) {
    refresh();
}

void RegistrationService::refresh() {
    registrationsById_.clear();
    waitlistsByEvent_.clear();

    // Repository results are ordered by creation time so FIFO waitlists rebuild consistently.
    for (const auto& registration : repository_.allRegistrations()) {
        registrationsById_[registration.id] = registration;
        if (registration.status == RegistrationStatus::Waitlisted) {
            waitlistsByEvent_[registration.eventId].push(registration.studentId);
        }
    }
}

int RegistrationService::confirmedCount(const std::string& eventId) const {
    int count = 0;
    for (const auto& entry : registrationsById_) {
        const auto& registration = entry.second;
        if (registration.eventId == eventId && registration.status == RegistrationStatus::Confirmed) {
            ++count;
        }
    }
    return count;
}

bool RegistrationService::hasActiveRegistration(const std::string& studentId, const std::string& eventId) const {
    for (const auto& entry : registrationsById_) {
        const auto& registration = entry.second;
        if (registration.studentId == studentId && registration.eventId == eventId &&
            registration.status != RegistrationStatus::Cancelled) {
            return true;
        }
    }
    return false;
}

Registration* RegistrationService::findActive(const std::string& studentId, const std::string& eventId) {
    for (auto& entry : registrationsById_) {
        auto& registration = entry.second;
        if (registration.studentId == studentId && registration.eventId == eventId &&
            registration.status != RegistrationStatus::Cancelled) {
            return &registration;
        }
    }
    return nullptr;
}

RegistrationStatus RegistrationService::registerStudent(const std::string& studentId, const std::string& eventId) {
    students_.get(studentId);
    const Event event = events_.get(eventId);

    if (hasActiveRegistration(studentId, eventId)) {
        throw DuplicateRegistrationException(studentId + " is already registered or waitlisted for " + eventId);
    }

    const auto status = confirmedCount(eventId) < event.capacity
        ? RegistrationStatus::Confirmed
        : RegistrationStatus::Waitlisted;
    Registration registration{makeId("REG"), studentId, eventId, status, nowTimestamp()};
    repository_.save(registration);
    registrationsById_[registration.id] = registration;

    if (status == RegistrationStatus::Waitlisted) {
        waitlistsByEvent_[eventId].push(studentId);
        notifications_.notify(studentId, "Event is full. You are on the FIFO waitlist for '" + event.title + "'.");
    } else {
        notifications_.notify(studentId, "Your registration for '" + event.title + "' is confirmed.");
    }
    return status;
}

void RegistrationService::cancel(const std::string& studentId, const std::string& eventId) {
    const Event event = events_.get(eventId);
    Registration* registration = findActive(studentId, eventId);
    if (!registration) {
        throw CampusConnectException("No active registration was found for this student and event.");
    }

    const bool releasesSeat = registration->status == RegistrationStatus::Confirmed;
    registration->status = RegistrationStatus::Cancelled;
    repository_.save(*registration);

    if (!releasesSeat) {
        // Cancelling a waiter frees no seat, so only remove that ID and preserve everyone else's order.
        const auto waitlist = waitlistsByEvent_.find(eventId);
        if (waitlist != waitlistsByEvent_.end()) {
            waitlist->second.remove(studentId);
        }
        return;
    }

    const auto waitlist = waitlistsByEvent_.find(eventId);
    if (waitlist != waitlistsByEvent_.end() && !waitlist->second.empty()) {
        // A released confirmed seat goes to the oldest remaining waiter.
        const std::string promotedStudentId = waitlist->second.pop();
        Registration* promoted = findActive(promotedStudentId, eventId);
        if (promoted && promoted->status == RegistrationStatus::Waitlisted) {
            promoted->status = RegistrationStatus::Confirmed;
            repository_.save(*promoted);
            notifications_.notify(promotedStudentId,
                "A seat opened. You have been promoted from the waitlist for '" + event.title + "'.");
        }
    }

    notifications_.notify(studentId, "Your registration for '" + event.title + "' has been cancelled.");
}

std::vector<Registration> RegistrationService::forStudent(const std::string& studentId) const {
    std::vector<Registration> registrations;
    for (const auto& entry : registrationsById_) {
        if (entry.second.studentId == studentId) {
            registrations.push_back(entry.second);
        }
    }
    std::sort(registrations.begin(), registrations.end(), [](const Registration& first, const Registration& second) {
        return first.createdAt < second.createdAt;
    });
    return registrations;
}

std::vector<std::string> RegistrationService::waitlistForEvent(const std::string& eventId) const {
    const auto waitlist = waitlistsByEvent_.find(eventId);
    return waitlist == waitlistsByEvent_.end() ? std::vector<std::string>{} : waitlist->second.toVector();
}
