#include "models/Models.h"

#include "exceptions/CampusConnectException.h"

std::string toString(RegistrationStatus status) {
    switch (status) {
        case RegistrationStatus::Confirmed: return "CONFIRMED";
        case RegistrationStatus::Waitlisted: return "WAITLISTED";
        case RegistrationStatus::Cancelled: return "CANCELLED";
    }
    return "CANCELLED";
}

RegistrationStatus registrationStatusFromString(const std::string& status) {
    if (status == "CONFIRMED") return RegistrationStatus::Confirmed;
    if (status == "WAITLISTED") return RegistrationStatus::Waitlisted;
    if (status == "CANCELLED") return RegistrationStatus::Cancelled;
    throw InvalidInputException("Unknown registration status: " + status);
}
