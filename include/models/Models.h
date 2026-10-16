#pragma once

#include <string>
#include <vector>

// Domain entities are plain C++ types. Service classes enforce business rules.
struct Student {
    std::string id;
    std::string name;
    std::string email;
    std::string department;
    std::vector<std::string> interests;
};

struct Club {
    std::string id;
    std::string name;
    std::string description;
    std::vector<std::string> memberIds;
    std::vector<std::string> interests;
};

struct Event {
    std::string id;
    std::string clubId;
    std::string title;
    std::string description;
    std::string date;      // YYYY-MM-DD, so lexical ordering is chronological.
    std::string time;      // HH:MM
    std::string venue;
    std::string category;
    int capacity = 0;
};

enum class RegistrationStatus { Confirmed, Waitlisted, Cancelled };
std::string toString(RegistrationStatus status);
RegistrationStatus registrationStatusFromString(const std::string& status);

struct Registration {
    std::string id;
    std::string studentId;
    std::string eventId;
    RegistrationStatus status = RegistrationStatus::Confirmed;
    std::string createdAt;
};

struct Attendance {
    std::string id;
    std::string studentId;
    std::string eventId;
    bool present = false;
    std::string markedAt;
};

struct Certificate {
    std::string id;
    std::string studentId;
    std::string eventId;
    std::string title;
    std::string issuedAt;
};

struct Feedback {
    std::string id;
    std::string studentId;
    std::string eventId;
    int rating = 0;
    std::string comment;
    std::string createdAt;
};

struct Notification {
    std::string id;
    std::string studentId;
    std::string message;
    std::string createdAt;
    bool read = false;
};

struct AccountCredential {
    std::string email;
    std::string displayName;
    std::string role;
    std::string subjectId;
    std::string passwordHash;
};
