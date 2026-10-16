#pragma once

#include "models/Models.h"

#include <filesystem>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/database.hpp>
#include <optional>
#include <string>
#include <vector>

// Single persistence boundary. MongoDB is the primary store; saveSnapshots()
// also writes the fstream text files required by the project report.
class Repository {
public:
    explicit Repository(const std::string& uri = "mongodb://127.0.0.1:27017/",
                        const std::string& databaseName = "campus_connect",
                        const std::filesystem::path& dataDirectory = "data");

    std::vector<Student> allStudents() const;
    std::vector<Club> allClubs() const;
    std::vector<Event> allEvents() const;
    std::vector<Registration> allRegistrations() const;
    std::vector<Attendance> allAttendance() const;
    std::vector<Certificate> allCertificates() const;
    std::vector<Feedback> allFeedback() const;
    std::vector<Notification> allNotifications() const;
    std::optional<AccountCredential> findAccountCredential(const std::string& email) const;

    void save(const Student& value);
    void save(const Club& value);
    void save(const Event& value);
    void save(const Registration& value);
    void save(const Attendance& value);
    void save(const Certificate& value);
    void save(const Feedback& value);
    void save(const Notification& value);
    void insertAccountCredential(const AccountCredential& value);

    void ensureDemoData();
    void saveSnapshots() const;

private:
    mongocxx::instance instance_;
    mongocxx::client client_;
    mongocxx::database database_;
    std::filesystem::path dataDirectory_;
};
