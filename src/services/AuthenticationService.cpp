#include "services/AuthenticationService.h"

#include "exceptions/CampusConnectException.h"
#include "utils/Utils.h"

#include <sodium.h>

#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace {
constexpr auto sessionLifetime = std::chrono::hours(8);
constexpr std::size_t minimumPasswordLength = 12;
}

AuthenticationService::AuthenticationService(Repository& repository, StudentService& students)
    : repository_(repository), students_(students) {
    if (sodium_init() < 0) {
        throw std::runtime_error("Could not initialize the password hashing library.");
    }
}

void AuthenticationService::provisionStudent(const std::string& studentId, const std::string& password) {
    const Student& student = students_.get(studentId);
    const std::string email = normalizeEmail(student.email);
    const auto atSign = email.find('@');
    if (atSign == std::string::npos || atSign == 0 || atSign == email.size() - 1) {
        throw InvalidInputException("The student profile does not have a valid email address.");
    }
    if (repository_.findAccountCredential(email)) {
        throw InvalidInputException("An account already exists for this email address.");
    }

    repository_.insertAccountCredential(
        AccountCredential{email, student.name, "student", student.id, hashPassword(password)});
}

void AuthenticationService::provisionCoordinator(const std::string& emailInput,
                                                 const std::string& displayName,
                                                 const std::string& password) {
    const std::string email = normalizeEmail(emailInput);
    const auto atSign = email.find('@');
    if (atSign == std::string::npos || atSign == 0 || atSign == email.size() - 1 || displayName.empty()) {
        throw InvalidInputException("A valid coordinator email and display name are required.");
    }
    if (repository_.findAccountCredential(email)) {
        throw InvalidInputException("An account already exists for this email address.");
    }

    repository_.insertAccountCredential(
        AccountCredential{email, displayName, "coordinator", email, hashPassword(password)});
}

AuthSession AuthenticationService::signUpStudent(const std::string& studentId,
                                                  const std::string& emailInput,
                                                  const std::string& password) {
    const Student& student = students_.get(studentId);
    const std::string email = normalizeEmail(emailInput);
    // Signup links an existing campus identity; it cannot create or rename student records.
    if (normalizeEmail(student.email) != email) {
        throw InvalidInputException("Student ID and campus email do not match an existing profile.");
    }

    provisionStudent(studentId, password);
    const auto session = login(email, password, "student");
    if (!session) {
        throw std::runtime_error("Could not start a session after account creation.");
    }
    return *session;
}

AuthSession AuthenticationService::signUpCoordinator(const std::string& email,
                                                     const std::string& displayName,
                                                     const std::string& password,
                                                     const std::string& inviteCode) {
    // The coordinator invite comes from the server environment, never from browser configuration.
    const char* expectedInviteCode = std::getenv("CAMPUS_CONNECT_COORDINATOR_INVITE");
    if (!expectedInviteCode || *expectedInviteCode == '\0' ||
        inviteCode.size() != std::strlen(expectedInviteCode) ||
        sodium_memcmp(inviteCode.data(), expectedInviteCode, inviteCode.size()) != 0) {
        throw InvalidInputException("Coordinator sign-up is closed or the invitation code is invalid.");
    }

    const std::string normalizedEmail = normalizeEmail(email);
    provisionCoordinator(normalizedEmail, displayName, password);
    const auto session = login(normalizedEmail, password, "coordinator");
    if (!session) {
        throw std::runtime_error("Could not start a session after account creation.");
    }
    return *session;
}

std::optional<AuthSession> AuthenticationService::login(const std::string& emailInput,
                                                        const std::string& password,
                                                        const std::string& expectedRole) {
    if (expectedRole != "student" && expectedRole != "coordinator") {
        return std::nullopt;
    }

    const std::string email = normalizeEmail(emailInput);
    const auto credential = repository_.findAccountCredential(email);
    if (!credential || credential->role != expectedRole ||
        crypto_pwhash_str_verify(credential->passwordHash.c_str(), password.c_str(), password.size()) != 0) {
        return std::nullopt;
    }

    AuthSession user{newSessionToken(), credential->email, credential->displayName,
                     credential->role, credential->subjectId};
    {
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        sessions_[user.token] = {user, std::chrono::steady_clock::now() + sessionLifetime};
    }
    return user;
}

std::optional<AuthSession> AuthenticationService::currentSession(const std::string& token) {
    if (token.empty()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(sessionsMutex_);
    const auto session = sessions_.find(token);
    if (session == sessions_.end()) {
        return std::nullopt;
    }
    if (session->second.expiresAt <= std::chrono::steady_clock::now()) {
        sessions_.erase(session);
        return std::nullopt;
    }
    return session->second.user;
}

void AuthenticationService::logout(const std::string& token) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    sessions_.erase(token);
}

std::string AuthenticationService::normalizeEmail(std::string email) {
    for (char& character : email) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return email;
}

std::string AuthenticationService::hashPassword(const std::string& password) {
    validatePassword(password);
    std::array<char, crypto_pwhash_STRBYTES> passwordHash{};
    // libsodium stores the Argon2id salt and work parameters in its encoded hash string.
    if (crypto_pwhash_str(passwordHash.data(), password.c_str(), password.size(),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        throw std::runtime_error("Could not hash the password; system memory may be low.");
    }
    return passwordHash.data();
}

void AuthenticationService::validatePassword(const std::string& password) {
    if (password.size() < minimumPasswordLength) {
        throw InvalidInputException("Passwords must contain at least 12 characters.");
    }
}

std::string AuthenticationService::newSessionToken() {
    std::array<unsigned char, 32> randomBytes{};
    std::array<char, 65> token{};
    // Return only the hex token; erase the random source bytes as soon as encoding is complete.
    randombytes_buf(randomBytes.data(), randomBytes.size());
    sodium_bin2hex(token.data(), token.size(), randomBytes.data(), randomBytes.size());
    sodium_memzero(randomBytes.data(), randomBytes.size());
    return token.data();
}