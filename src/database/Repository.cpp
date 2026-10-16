#include "database/Repository.h"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/options/index.hpp>
#include <mongocxx/options/replace.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

using bsoncxx::builder::basic::array;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::kvp;

namespace {
std::string readString(const bsoncxx::document::view& document, const char* key) {
    const auto value = document[key];
    if (!value || value.type() != bsoncxx::type::k_string) return {};
    const auto view = value.get_string().value;
    return std::string(view.data(), view.size());
}

int readInt(const bsoncxx::document::view& document, const char* key) {
    const auto value = document[key];
    if (!value) return 0;
    if (value.type() == bsoncxx::type::k_int32) return value.get_int32().value;
    if (value.type() == bsoncxx::type::k_int64) return static_cast<int>(value.get_int64().value);
    return 0;
}

bool readBool(const bsoncxx::document::view& document, const char* key) {
    const auto value = document[key];
    return value && value.type() == bsoncxx::type::k_bool && value.get_bool().value;
}

std::vector<std::string> readStringArray(const bsoncxx::document::view& document, const char* key) {
    std::vector<std::string> result;
    const auto value = document[key];
    if (!value || value.type() != bsoncxx::type::k_array) return result;
    for (const auto& item : value.get_array().value) {
        if (item.type() == bsoncxx::type::k_string) {
            const auto view = item.get_string().value;
            result.emplace_back(view.data(), view.size());
        }
    }
    return result;
}

array makeStringArray(const std::vector<std::string>& values) {
    array result;
    for (const auto& value : values) result.append(value);
    return result;
}

std::string safeText(std::string value) {
    for (char& ch : value) {
        if (ch == '|') ch = '/';
        if (ch == '\r' || ch == '\n') ch = ' ';
    }
    return value;
}

template <typename Range, typename Formatter>
void writeRows(const std::filesystem::path& path, const Range& values, Formatter format) {
    std::ofstream output(path, std::ios::trunc);
    if (!output) throw std::runtime_error("Could not write data snapshot: " + path.string());
    for (const auto& value : values) output << format(value) << '\n';
}
} // namespace

Repository::Repository(const std::string& uri, const std::string& databaseName,
                       const std::filesystem::path& dataDirectory)
    : client_(mongocxx::uri{uri}), database_(client_[databaseName]), dataDirectory_(dataDirectory) {
    std::filesystem::create_directories(dataDirectory_);
    database_["accounts"].create_index(make_document(kvp("email", 1)),
                                        mongocxx::options::index{}.unique(true));
    ensureDemoData();
}

std::vector<Student> Repository::allStudents() const {
    std::vector<Student> result;
    for (const auto& document : database_["students"].find(make_document())) {
        result.push_back({readString(document, "id"), readString(document, "name"),
                          readString(document, "email"), readString(document, "department"),
                          readStringArray(document, "interests")});
    }
    return result;
}

std::vector<Club> Repository::allClubs() const {
    std::vector<Club> result;
    for (const auto& document : database_["clubs"].find(make_document())) {
        result.push_back({readString(document, "id"), readString(document, "name"),
                          readString(document, "description"), readStringArray(document, "memberIds"),
                          readStringArray(document, "interests")});
    }
    return result;
}

std::vector<Event> Repository::allEvents() const {
    std::vector<Event> result;
    for (const auto& document : database_["events"].find(make_document())) {
        result.push_back({readString(document, "id"), readString(document, "clubId"),
                          readString(document, "title"), readString(document, "description"),
                          readString(document, "date"), readString(document, "time"),
                          readString(document, "venue"), readString(document, "category"),
                          readInt(document, "capacity")});
    }
    return result;
}

std::vector<Registration> Repository::allRegistrations() const {
    std::vector<Registration> result;
    for (const auto& document : database_["registrations"].find(make_document())) {
        result.push_back({readString(document, "id"), readString(document, "studentId"),
                          readString(document, "eventId"),
                          registrationStatusFromString(readString(document, "status")),
                          readString(document, "createdAt")});
    }
    std::sort(result.begin(), result.end(), [](const Registration& a, const Registration& b) {
        if (a.createdAt != b.createdAt) return a.createdAt < b.createdAt;
        return a.id < b.id; // deterministic FIFO tie-breaker for identical timestamps.
    });
    return result;
}

std::vector<Attendance> Repository::allAttendance() const {
    std::vector<Attendance> result;
    for (const auto& document : database_["attendance"].find(make_document())) {
        result.push_back({readString(document, "id"), readString(document, "studentId"),
                          readString(document, "eventId"), readBool(document, "present"),
                          readString(document, "markedAt")});
    }
    return result;
}

std::vector<Certificate> Repository::allCertificates() const {
    std::vector<Certificate> result;
    for (const auto& document : database_["certificates"].find(make_document())) {
        result.push_back({readString(document, "id"), readString(document, "studentId"),
                          readString(document, "eventId"), readString(document, "title"),
                          readString(document, "issuedAt")});
    }
    return result;
}

std::vector<Feedback> Repository::allFeedback() const {
    std::vector<Feedback> result;
    for (const auto& document : database_["feedback"].find(make_document())) {
        result.push_back({readString(document, "id"), readString(document, "studentId"),
                          readString(document, "eventId"), readInt(document, "rating"),
                          readString(document, "comment"), readString(document, "createdAt")});
    }
    return result;
}

std::vector<Notification> Repository::allNotifications() const {
    std::vector<Notification> result;
    for (const auto& document : database_["notifications"].find(make_document())) {
        result.push_back({readString(document, "id"), readString(document, "studentId"),
                          readString(document, "message"), readString(document, "createdAt"),
                          readBool(document, "read")});
    }
    std::sort(result.begin(), result.end(), [](const Notification& a, const Notification& b) {
        if (a.createdAt != b.createdAt) return a.createdAt < b.createdAt;
        return a.id < b.id;
    });
    return result;
}

std::optional<AccountCredential> Repository::findAccountCredential(const std::string& email) const {
    const auto document = database_["accounts"].find_one(make_document(kvp("email", email)));
    if (!document) {
        return std::nullopt;
    }

    const auto view = document->view();
    return AccountCredential{readString(view, "email"), readString(view, "displayName"),
                             readString(view, "role"), readString(view, "subjectId"),
                             readString(view, "passwordHash")};
}

void Repository::save(const Student& value) {
    database_["students"].replace_one(make_document(kvp("id", value.id)),
        make_document(kvp("id", value.id), kvp("name", value.name), kvp("email", value.email),
                      kvp("department", value.department), kvp("interests", makeStringArray(value.interests))),
        mongocxx::options::replace{}.upsert(true));
}

void Repository::save(const Club& value) {
    database_["clubs"].replace_one(make_document(kvp("id", value.id)),
        make_document(kvp("id", value.id), kvp("name", value.name), kvp("description", value.description),
                      kvp("memberIds", makeStringArray(value.memberIds)), kvp("interests", makeStringArray(value.interests))),
        mongocxx::options::replace{}.upsert(true));
}

void Repository::save(const Event& value) {
    database_["events"].replace_one(make_document(kvp("id", value.id)),
        make_document(kvp("id", value.id), kvp("clubId", value.clubId), kvp("title", value.title),
                      kvp("description", value.description), kvp("date", value.date), kvp("time", value.time),
                      kvp("venue", value.venue), kvp("category", value.category), kvp("capacity", value.capacity)),
        mongocxx::options::replace{}.upsert(true));
}

void Repository::save(const Registration& value) {
    database_["registrations"].replace_one(make_document(kvp("id", value.id)),
        make_document(kvp("id", value.id), kvp("studentId", value.studentId), kvp("eventId", value.eventId),
                      kvp("status", toString(value.status)), kvp("createdAt", value.createdAt)),
        mongocxx::options::replace{}.upsert(true));
}

void Repository::save(const Attendance& value) {
    database_["attendance"].replace_one(make_document(kvp("id", value.id)),
        make_document(kvp("id", value.id), kvp("studentId", value.studentId), kvp("eventId", value.eventId),
                      kvp("present", value.present), kvp("markedAt", value.markedAt)),
        mongocxx::options::replace{}.upsert(true));
}

void Repository::save(const Certificate& value) {
    database_["certificates"].replace_one(make_document(kvp("id", value.id)),
        make_document(kvp("id", value.id), kvp("studentId", value.studentId), kvp("eventId", value.eventId),
                      kvp("title", value.title), kvp("issuedAt", value.issuedAt)),
        mongocxx::options::replace{}.upsert(true));
}

void Repository::save(const Feedback& value) {
    database_["feedback"].replace_one(make_document(kvp("id", value.id)),
        make_document(kvp("id", value.id), kvp("studentId", value.studentId), kvp("eventId", value.eventId),
                      kvp("rating", value.rating), kvp("comment", value.comment), kvp("createdAt", value.createdAt)),
        mongocxx::options::replace{}.upsert(true));
}

void Repository::save(const Notification& value) {
    database_["notifications"].replace_one(make_document(kvp("id", value.id)),
        make_document(kvp("id", value.id), kvp("studentId", value.studentId), kvp("message", value.message),
                      kvp("createdAt", value.createdAt), kvp("read", value.read)),
        mongocxx::options::replace{}.upsert(true));
}

    void Repository::insertAccountCredential(const AccountCredential& value) {
        database_["accounts"].insert_one(
            make_document(kvp("email", value.email), kvp("displayName", value.displayName),
                          kvp("role", value.role), kvp("subjectId", value.subjectId),
                          kvp("passwordHash", value.passwordHash)));
    }

void Repository::ensureDemoData() {
    if (allStudents().empty()) {
        save(Student{"S1001", "Aarav Sharma", "aarav@campus.local", "CSE", {"coding", "AI", "robotics"}});
        save(Student{"S1002", "Priya Singh", "priya@campus.local", "CSE", {"coding", "music", "design"}});
        save(Student{"S1003", "Rohan Verma", "rohan@campus.local", "ECE", {"robotics", "electronics"}});
    }
    if (allClubs().empty()) {
        save(Club{"C101", "Coding Club", "Programming and software projects", {"S1001", "S1002"}, {"coding", "software", "AI"}});
        save(Club{"C102", "Robotics Club", "Robotics and embedded systems", {"S1003"}, {"robotics", "electronics"}});
        save(Club{"C103", "Design Club", "UI/UX and creative design", {}, {"design", "art"}});
    }
    if (allEvents().empty()) {
        save(Event{"E101", "C101", "DSA Workshop", "Hands-on data structures workshop", "2026-11-05", "10:00", "Lab 3", "Technical", 2});
        save(Event{"E102", "C101", "Campus Hackathon", "A campus coding challenge", "2026-11-12", "09:00", "Innovation Hall", "Technical", 50});
        save(Event{"E103", "C102", "Robotics Showcase", "Student robotics projects", "2026-11-15", "14:00", "Auditorium", "Robotics", 30});
        save(Event{"E104", "C103", "Design Jam", "Collaborative UX challenge", "2026-11-20", "11:00", "Studio 2", "Design", 25});
    }
}

void Repository::saveSnapshots() const {
    writeRows(dataDirectory_ / "students.txt", allStudents(), [](const Student& x) {
        return safeText(x.id) + "|" + safeText(x.name) + "|" + safeText(x.email) + "|" + safeText(x.department);
    });
    writeRows(dataDirectory_ / "clubs.txt", allClubs(), [](const Club& x) {
        return safeText(x.id) + "|" + safeText(x.name) + "|" + safeText(x.description);
    });
    writeRows(dataDirectory_ / "events.txt", allEvents(), [](const Event& x) {
        return safeText(x.id) + "|" + safeText(x.clubId) + "|" + safeText(x.title) + "|" +
               safeText(x.date) + "|" + safeText(x.time) + "|" + std::to_string(x.capacity) + "|" +
               safeText(x.venue) + "|" + safeText(x.category) + "|" + safeText(x.description);
    });
    writeRows(dataDirectory_ / "registration.txt", allRegistrations(), [](const Registration& x) {
        return x.id + "|" + x.studentId + "|" + x.eventId + "|" + toString(x.status) + "|" + x.createdAt;
    });
    writeRows(dataDirectory_ / "attendance.txt", allAttendance(), [](const Attendance& x) {
        return x.id + "|" + x.studentId + "|" + x.eventId + "|" + (x.present ? "PRESENT" : "ABSENT") + "|" + x.markedAt;
    });
    writeRows(dataDirectory_ / "certificates.txt", allCertificates(), [](const Certificate& x) {
        return x.id + "|" + x.studentId + "|" + x.eventId + "|" + safeText(x.title) + "|" + x.issuedAt;
    });
    writeRows(dataDirectory_ / "feedback.txt", allFeedback(), [](const Feedback& x) {
        return x.id + "|" + x.studentId + "|" + x.eventId + "|" + std::to_string(x.rating) + "|" + safeText(x.comment);
    });
    writeRows(dataDirectory_ / "notifications.txt", allNotifications(), [](const Notification& x) {
        return x.id + "|" + x.studentId + "|" + safeText(x.message) + "|" + x.createdAt + "|" + (x.read ? "READ" : "UNREAD");
    });
}
