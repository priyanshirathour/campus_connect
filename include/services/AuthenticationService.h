#pragma once

#include "database/Repository.h"
#include "services/StudentService.h"

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

struct AuthSession {
    std::string token;
    std::string email;
    std::string displayName;
    std::string role;
    std::string subjectId;
};

// Stores password hashes and validates role-bound, expiring web sessions.
class AuthenticationService {
public:
    AuthenticationService(Repository& repository, StudentService& students);

    void provisionStudent(const std::string& studentId, const std::string& password);
    void provisionCoordinator(const std::string& email, const std::string& displayName,
                              const std::string& password);
    AuthSession signUpStudent(const std::string& studentId, const std::string& email,
                              const std::string& password);
    AuthSession signUpCoordinator(const std::string& email, const std::string& displayName,
                                 const std::string& password, const std::string& inviteCode);
    std::optional<AuthSession> login(const std::string& email, const std::string& password,
                                     const std::string& expectedRole);
    std::optional<AuthSession> currentSession(const std::string& token);
    void logout(const std::string& token);

private:
    struct StoredSession {
        AuthSession user;
        std::chrono::steady_clock::time_point expiresAt;
    };

    Repository& repository_;
    StudentService& students_;
    // Sessions are intentionally process-local; restarting the API invalidates all cookies.
    std::mutex sessionsMutex_;
    std::unordered_map<std::string, StoredSession> sessions_;

    static std::string normalizeEmail(std::string email);
    static std::string hashPassword(const std::string& password);
    static void validatePassword(const std::string& password);
    static std::string newSessionToken();
};