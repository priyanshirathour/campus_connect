#include "CampusConnectApp.h"
#include "exceptions/CampusConnectException.h"
#include "utils/Utils.h"
#include "web/WebApiServer.h"

#include <iomanip>
#include <iostream>
#include <limits>
#include <sodium.h>

namespace {
void clearSecret(std::string& secret) {
    if (!secret.empty()) {
        sodium_memzero(secret.data(), secret.size());
        secret.clear();
    }
}

bool promptForPassword(std::string& password) {
    password = inputSecret("Password (12 characters minimum): ");
    std::string confirmation = inputSecret("Confirm password: ");
    const bool matches = password == confirmation;
    clearSecret(confirmation);
    if (!matches) {
        clearSecret(password);
        std::cerr << "Passwords did not match. No account was created.\n";
    }
    return matches;
}
}

CampusConnectApp::CampusConnectApp()
    : repository_(), students_(repository_), authentication_(repository_, students_),
      clubs_(repository_, students_), events_(repository_, clubs_),
      notifications_(repository_), registrations_(repository_, students_, events_, notifications_),
      attendance_(repository_, students_, events_, registrations_),
      certificates_(repository_, students_, events_, attendance_),
      feedback_(repository_, students_, events_), recommendations_(repository_, students_, clubs_), reports_(repository_) {}

void CampusConnectApp::listEvents() const {
    const auto events = events_.sortedByDate();
    std::cout << "\nID     | Date       | Time  | Event                 | Seats | Category\n";
    std::cout << "-------+------------+-------+-----------------------+-------+-----------\n";
    for (const auto& event : events) {
        const int remaining = event.capacity - registrations_.confirmedCount(event.id);
        std::cout << std::left << std::setw(6) << event.id << " | " << std::setw(10) << event.date << " | "
                  << std::setw(5) << event.time << " | " << std::setw(21) << event.title.substr(0, 21)
                  << " | " << std::setw(2) << registrations_.confirmedCount(event.id) << "/" << std::setw(3) << event.capacity
                  << " | " << event.category << (remaining <= 0 ? " [FULL]" : "") << '\n';
    }
}

void CampusConnectApp::listStudents() const {
    std::cout << "\nStudents:\n";
    for (const auto& student : students_.all())
        std::cout << "  " << student.id << " | " << student.name << " | " << student.department << '\n';
}

void CampusConnectApp::listClubs() const {
    std::cout << "\nClubs:\n";
    for (const auto& club : clubs_.all())
        std::cout << "  " << club.id << " | " << club.name << " | members: " << club.memberIds.size() << '\n';
}

void CampusConnectApp::studentMenu() {
    listStudents();
    const std::string studentId = inputLine("Enter your student ID: ");
    students_.get(studentId); // fail early with a clear exception if the ID is invalid.
    while (true) {
        std::cout << "\n--- STUDENT MENU (" << studentId << ") ---\n"
                  << "1. Browse events\n2. Search events\n3. Register for event\n4. Cancel registration\n"
                  << "5. My registrations\n6. View an event waitlist\n7. View notifications\n"
                  << "8. Recommendations\n9. My certificates\n10. Submit feedback\n0. Back\n";
        const int choice = inputInt("Choice: ", 0, 10);
        try {
            if (choice == 0) return;
            if (choice == 1) listEvents();
            else if (choice == 2) {
                const auto query = inputLine("Search title/category/description: ");
                for (const auto& event : events_.search(query))
                    std::cout << event.id << " | " << event.title << " | " << event.date << '\n';
            } else if (choice == 3) {
                const auto status = registrations_.registerStudent(studentId, inputLine("Event ID: "));
                std::cout << "Registration status: " << toString(status) << '\n';
            } else if (choice == 4) {
                registrations_.cancel(studentId, inputLine("Event ID: "));
                std::cout << "Registration cancelled. If a confirmed seat was released, the first waiter was promoted.\n";
            } else if (choice == 5) {
                for (const auto& registration : registrations_.forStudent(studentId))
                    std::cout << registration.eventId << " | " << toString(registration.status) << " | " << registration.createdAt << '\n';
            } else if (choice == 6) {
                const auto eventId = inputLine("Event ID: "); events_.get(eventId);
                const auto ids = registrations_.waitlistForEvent(eventId);
                if (ids.empty()) std::cout << "Waitlist is empty.\n";
                for (std::size_t i=0;i<ids.size();++i) std::cout << (i+1) << ". " << ids[i] << '\n';
            } else if (choice == 7) {
                const auto items = notifications_.forStudent(studentId);
                if (items.empty()) std::cout << "No unread notifications.\n";
                for (const auto& n : items) std::cout << "- " << n.createdAt << " | " << n.message << '\n';
                notifications_.markAllRead(studentId);
            } else if (choice == 8) {
                std::cout << "Recommended clubs:\n";
                const auto clubs = recommendations_.recommendClubs(studentId);
                if (clubs.empty()) std::cout << "  No matches yet; add interests or browse all clubs.\n";
                for (const auto& club : clubs) std::cout << "  " << club.id << " | " << club.name << '\n';
                std::cout << "Recommended events:\n";
                for (const auto& event : recommendations_.recommendEvents(studentId)) std::cout << "  " << event.id << " | " << event.title << '\n';
            } else if (choice == 9) {
                const auto items = certificates_.forStudent(studentId);
                if (items.empty()) std::cout << "No certificates yet.\n";
                for (const auto& cert : items) std::cout << cert.id << " | " << cert.title << " | " << cert.issuedAt << '\n';
            } else if (choice == 10) {
                const auto eventId = inputLine("Event ID: ");
                const int rating = inputInt("Rating (1-5): ", 1, 5);
                feedback_.submit(studentId, eventId, rating, inputLine("Comment: "));
                std::cout << "Feedback saved. Thank you!\n";
            }
        } catch (const CampusConnectException& error) {
            std::cout << "Could not complete that action: " << error.what() << '\n';
        }
    }
}

void CampusConnectApp::coordinatorMenu() {
    while (true) {
        std::cout << "\n--- COORDINATOR MENU ---\n1. Browse events\n2. Add event\n3. Add club\n4. Add student to club\n"
                  << "5. Mark attendance\n6. Issue certificate\n7. Feedback report\n8. System summary\n9. Export summary\n0. Back\n";
        const int choice = inputInt("Choice: ", 0, 9);
        try {
            if (choice == 0) return;
            if (choice == 1) listEvents();
            else if (choice == 2) {
                Event event;
                event.id = inputLine("Event ID (example E105): ");
                event.clubId = inputLine("Club ID: ");
                event.title = inputLine("Title: ");
                event.description = inputLine("Description: ");
                event.date = inputLine("Date YYYY-MM-DD: ");
                event.time = inputLine("Time HH:MM: ");
                event.venue = inputLine("Venue: ");
                event.category = inputLine("Category: ");
                event.capacity = inputInt("Capacity: ", 1, 100000);
                events_.add(event); recommendations_.rebuild(); std::cout << "Event created.\n";
            } else if (choice == 3) {
                Club club;
                club.id = inputLine("Club ID: "); club.name = inputLine("Club name: ");
                club.description = inputLine("Description: ");
                std::string tags = inputLine("Interests (comma-separated): ");
                std::string token;
                for (char ch : tags) { if (ch == ',') { if (!token.empty()) club.interests.push_back(token); token.clear(); } else token += ch; }
                if (!token.empty()) club.interests.push_back(token);
                clubs_.add(club); recommendations_.rebuild(); std::cout << "Club created.\n";
            } else if (choice == 4) {
                listClubs(); clubs_.addMember(inputLine("Club ID: "), inputLine("Student ID: "));
                recommendations_.rebuild(); std::cout << "Club membership updated.\n";
            } else if (choice == 5) {
                const auto studentId = inputLine("Student ID: "); const auto eventId = inputLine("Event ID: ");
                const bool present = inputInt("1 = present, 0 = absent: ", 0, 1) == 1;
                attendance_.mark(studentId, eventId, present); std::cout << "Attendance saved.\n";
            } else if (choice == 6) {
                const auto cert = certificates_.issue(inputLine("Student ID: "), inputLine("Event ID: "));
                std::cout << "Certificate issued: " << cert.id << " | " << cert.title << '\n';
            } else if (choice == 7) {
                const auto eventId = inputLine("Event ID: ");
                std::cout << "Average rating: " << std::fixed << std::setprecision(2) << feedback_.averageRating(eventId) << '\n';
                for (const auto& item : feedback_.forEvent(eventId)) std::cout << item.rating << "/5 | " << item.comment << '\n';
            } else if (choice == 8) reports_.printSummary();
            else if (choice == 9) { const auto path=inputLine("Output path [report.txt]: "); reports_.exportSummary(path.empty()?"report.txt":path); std::cout << "Summary exported.\n"; }
        } catch (const CampusConnectException& error) {
            std::cout << "Could not complete that action: " << error.what() << '\n';
        }
    }
}

int CampusConnectApp::runApi() {
    WebApiServer api(events_, clubs_, authentication_, students_, registrations_, notifications_,
                     recommendations_, attendance_, certificates_, feedback_, reports_);
    std::cout << "Campus Connect API listening at http://127.0.0.1:8081\n";
    if (!api.listen()) {
        std::cerr << "Could not start the API server. Check whether port 8081 is already in use.\n";
        return 1;
    }
    return 0;
}

int CampusConnectApp::provisionStudentAccount(const std::string& studentId) {
    std::string password;
    if (!promptForPassword(password)) {
        return 1;
    }

    try {
        authentication_.provisionStudent(studentId, password);
        clearSecret(password);
    } catch (...) {
        clearSecret(password);
        throw;
    }
    std::cout << "Student login provisioned for " << studentId << ".\n";
    return 0;
}

int CampusConnectApp::provisionCoordinatorAccount(const std::string& email, const std::string& displayName) {
    std::string password;
    if (!promptForPassword(password)) {
        return 1;
    }

    try {
        authentication_.provisionCoordinator(email, displayName, password);
        clearSecret(password);
    } catch (...) {
        clearSecret(password);
        throw;
    }
    std::cout << "Coordinator login provisioned for " << email << ".\n";
    return 0;
}

int CampusConnectApp::run() {
    std::cout << "\n============================================\n"
              << " CAMPUS CONNECT\n Smart Club & Event Management System\n"
              << "============================================\n"
              << "Connected to MongoDB database: campus_connect\n";
    bool running = true;
    while (running) {
        std::cout << "\nMAIN MENU\n1. Student view\n2. Coordinator view\n3. Browse events\n4. System report\n0. Save snapshots and exit\n";
        const int choice = inputInt("Choice: ", 0, 4);
        try {
            if (choice == 0) running = false;
            else if (choice == 1) studentMenu();
            else if (choice == 2) coordinatorMenu();
            else if (choice == 3) listEvents();
            else reports_.printSummary();
        } catch (const CampusConnectException& error) {
            std::cout << "Action failed: " << error.what() << '\n';
        } catch (const std::exception& error) {
            std::cout << "Unexpected error: " << error.what() << '\n';
        }
    }
    repository_.saveSnapshots();
    std::cout << "Text snapshots saved in the data folder. Goodbye!\n";
    return 0;
}
