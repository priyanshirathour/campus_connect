#pragma once

#include <stdexcept>
#include <string>

// One base type lets the UI handle expected business-rule failures consistently.
class CampusConnectException : public std::runtime_error {
public:
    explicit CampusConnectException(const std::string& message)
        : std::runtime_error(message) {}
};

class StudentNotFoundException : public CampusConnectException {
public:
    explicit StudentNotFoundException(const std::string& id)
        : CampusConnectException("Student not found: " + id) {}
};

class ClubNotFoundException : public CampusConnectException {
public:
    explicit ClubNotFoundException(const std::string& id)
        : CampusConnectException("Club not found: " + id) {}
};

class EventNotFoundException : public CampusConnectException {
public:
    explicit EventNotFoundException(const std::string& id)
        : CampusConnectException("Event not found: " + id) {}
};

class DuplicateRegistrationException : public CampusConnectException {
public:
    explicit DuplicateRegistrationException(const std::string& message)
        : CampusConnectException("Duplicate registration: " + message) {}
};

class CertificateNotFoundException : public CampusConnectException {
public:
    explicit CertificateNotFoundException(const std::string& id)
        : CampusConnectException("Certificate not found: " + id) {}
};

class InvalidInputException : public CampusConnectException {
public:
    explicit InvalidInputException(const std::string& message)
        : CampusConnectException(message) {}
};
