#pragma once
#include "database/Repository.h"
#include "services/AuthenticationService.h"
#include "services/AttendanceService.h"
#include "services/CertificateService.h"
#include "services/ClubService.h"
#include "services/EventService.h"
#include "services/FeedbackService.h"
#include "services/NotificationService.h"
#include "services/RecommendationService.h"
#include "services/RegistrationService.h"
#include "services/ReportService.h"
#include "services/StudentService.h"

class CampusConnectApp {
public:
    CampusConnectApp();
    int run();
    int runApi();
    int provisionStudentAccount(const std::string& studentId);
    int provisionCoordinatorAccount(const std::string& email, const std::string& displayName);
private:
    Repository repository_;
    StudentService students_;
    AuthenticationService authentication_;
    ClubService clubs_;
    EventService events_;
    NotificationService notifications_;
    RegistrationService registrations_;
    AttendanceService attendance_;
    CertificateService certificates_;
    FeedbackService feedback_;
    RecommendationService recommendations_;
    ReportService reports_;

    void listEvents() const;
    void studentMenu();
    void coordinatorMenu();
    void listStudents() const;
    void listClubs() const;
};
