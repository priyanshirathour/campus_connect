#pragma once

class ClubService;
class EventService;
class AuthenticationService;
class StudentService;
class RegistrationService;
class NotificationService;
class RecommendationService;
class AttendanceService;
class CertificateService;
class FeedbackService;
class ReportService;

// Exposes read-only event and club data from the C++ service layer on loopback.
class WebApiServer {
public:
    WebApiServer(EventService& events, ClubService& clubs, AuthenticationService& authentication,
                 StudentService& students, RegistrationService& registrations,
                 NotificationService& notifications, RecommendationService& recommendations,
                 AttendanceService& attendance, CertificateService& certificates,
                 FeedbackService& feedback, ReportService& reports);
    bool listen(int port = 8081);

private:
    EventService& events_;
    ClubService& clubs_;
    AuthenticationService& authentication_;
    StudentService& students_;
    RegistrationService& registrations_;
    NotificationService& notifications_;
    RecommendationService& recommendations_;
    AttendanceService& attendance_;
    CertificateService& certificates_;
    FeedbackService& feedback_;
    ReportService& reports_;
};