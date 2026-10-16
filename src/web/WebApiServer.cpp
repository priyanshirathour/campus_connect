#include "web/WebApiServer.h"

#include "exceptions/CampusConnectException.h"
#include "models/Models.h"
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

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <cctype>
#include <algorithm>
#include <optional>
#include <string>

namespace {
nlohmann::json toJson(const Event& event) {
    return {
        {"id", event.id},
        {"clubId", event.clubId},
        {"title", event.title},
        {"description", event.description},
        {"date", event.date},
        {"time", event.time},
        {"venue", event.venue},
        {"category", event.category},
        {"capacity", event.capacity}
    };
}

nlohmann::json toJson(const Club& club) {
    return {
        {"id", club.id},
        {"name", club.name},
        {"description", club.description},
        {"memberCount", club.memberIds.size()},
        {"interests", club.interests}
    };
}

std::string sessionTokenFromCookie(const httplib::Request& request) {
    const std::string cookies = request.get_header_value("Cookie");
    const std::string name = "campus_session=";
    std::size_t start = 0;

    while (start < cookies.size()) {
        while (start < cookies.size() && (cookies[start] == ';' || std::isspace(static_cast<unsigned char>(cookies[start])))) {
            ++start;
        }
        const auto end = cookies.find(';', start);
        const auto cookie = cookies.substr(start, end == std::string::npos ? end : end - start);
        if (cookie.compare(0, name.size(), name) == 0) {
            return cookie.substr(name.size());
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return {};
}

nlohmann::json toJson(const AuthSession& session) {
    return {
        {"authenticated", true},
        {"email", session.email},
        {"displayName", session.displayName},
        {"role", session.role},
        {"subjectId", session.subjectId}
    };
}

nlohmann::json toJson(const Registration& registration) {
    return {
        {"id", registration.id},
        {"eventId", registration.eventId},
        {"status", toString(registration.status)},
        {"createdAt", registration.createdAt}
    };
}

nlohmann::json toJson(const Notification& notification) {
    return {
        {"id", notification.id},
        {"message", notification.message},
        {"createdAt", notification.createdAt},
        {"read", notification.read}
    };
}

nlohmann::json toJson(const Certificate& certificate) {
    return {
        {"id", certificate.id},
        {"eventId", certificate.eventId},
        {"title", certificate.title},
        {"issuedAt", certificate.issuedAt}
    };
}

nlohmann::json toJson(const Feedback& feedback) {
    return {
        {"id", feedback.id},
        {"studentId", feedback.studentId},
        {"rating", feedback.rating},
        {"comment", feedback.comment},
        {"createdAt", feedback.createdAt}
    };
}

std::optional<AuthSession> requireRole(AuthenticationService& authentication,
                                       const httplib::Request& request,
                                       httplib::Response& response,
                                       const std::string& role) {
    const auto session = authentication.currentSession(sessionTokenFromCookie(request));
    if (!session) {
        response.status = 401;
        response.set_content(R"({"error":"Sign in to continue."})", "application/json; charset=utf-8");
        return std::nullopt;
    }
    if (session->role != role) {
        response.status = 403;
        response.set_content(R"({"error":"This action is not available for this account type."})",
                             "application/json; charset=utf-8");
        return std::nullopt;
    }
    return session;
}

std::optional<nlohmann::json> parseObject(const httplib::Request& request,
                                         httplib::Response& response) {
    auto body = nlohmann::json::parse(request.body, nullptr, false);
    if (body.is_discarded() || !body.is_object()) {
        response.status = 400;
        response.set_content(R"({"error":"A JSON object is required."})", "application/json; charset=utf-8");
        return std::nullopt;
    }
    return body;
}

void setServiceError(httplib::Response& response, const std::exception& error) {
    response.status = dynamic_cast<const CampusConnectException*>(&error) ? 400 : 500;
    const std::string message = response.status == 400
        ? error.what()
        : "The request could not be completed.";
    response.set_content(nlohmann::json{{"error", message}}.dump(), "application/json; charset=utf-8");
}

void setSessionCookie(httplib::Response& response, const std::string& token) {
    // Keep the session token out of JavaScript and prevent cross-site cookie submission.
    response.set_header("Set-Cookie", "campus_session=" + token +
        "; HttpOnly; SameSite=Strict; Path=/; Max-Age=28800");
    response.set_header("Cache-Control", "no-store");
}
} // namespace

WebApiServer::WebApiServer(EventService& events, ClubService& clubs,
                                                     AuthenticationService& authentication, StudentService& students,
                                                     RegistrationService& registrations, NotificationService& notifications,
                                                     RecommendationService& recommendations, AttendanceService& attendance,
                                                     CertificateService& certificates, FeedbackService& feedback,
                                                     ReportService& reports)
        : events_(events), clubs_(clubs), authentication_(authentication), students_(students),
            registrations_(registrations), notifications_(notifications), recommendations_(recommendations),
            attendance_(attendance), certificates_(certificates), feedback_(feedback), reports_(reports) {}

bool WebApiServer::listen(int port) {
    httplib::Server server;
    // Service indexes are process-local and not thread-safe; serialize API handlers until they gain locks.
    server.new_task_queue = [] { return new httplib::ThreadPool(1); };
    server.set_payload_max_length(8 * 1024);

    // Public discovery routes serialize only the fields the website needs, not raw database documents.
    server.Get("/api/health", [](const httplib::Request&, httplib::Response& response) {
        response.set_content(R"({"status":"ok"})", "application/json; charset=utf-8");
    });

    server.Get("/api/events", [this](const httplib::Request& request, httplib::Response& response) {
        const auto query = request.has_param("q") ? request.get_param_value("q") : std::string{};
        const auto events = query.empty() ? events_.sortedByDate() : events_.search(query);
        auto payload = nlohmann::json::array();
        for (const auto& event : events) {
            payload.push_back(toJson(event));
        }
        response.set_content(payload.dump(), "application/json; charset=utf-8");
    });

    server.Get("/api/clubs", [this](const httplib::Request&, httplib::Response& response) {
        auto payload = nlohmann::json::array();
        for (const auto& club : clubs_.all()) {
            payload.push_back(toJson(club));
        }
        response.set_content(payload.dump(), "application/json; charset=utf-8");
    });

    server.Get("/api/student/dashboard", [this](const httplib::Request& request, httplib::Response& response) {
        const auto session = requireRole(authentication_, request, response, "student");
        if (!session) return;

        try {
            const Student& student = students_.get(session->subjectId);
            auto registrations = nlohmann::json::array();
            for (const auto& registration : registrations_.forStudent(student.id)) {
                auto item = toJson(registration);
                if (registration.status == RegistrationStatus::Waitlisted) {
                    const auto waitlist = registrations_.waitlistForEvent(registration.eventId);
                    const auto position = std::find(waitlist.begin(), waitlist.end(), student.id);
                    if (position != waitlist.end()) {
                        item["waitlistPosition"] = static_cast<std::size_t>(position - waitlist.begin()) + 1;
                    }
                }
                registrations.push_back(std::move(item));
            }

            auto notifications = nlohmann::json::array();
            for (const auto& notification : notifications_.forStudent(student.id)) {
                notifications.push_back(toJson(notification));
            }

            auto recommendedClubs = nlohmann::json::array();
            for (const auto& club : recommendations_.recommendClubs(student.id)) {
                recommendedClubs.push_back(toJson(club));
            }

            auto recommendedEvents = nlohmann::json::array();
            for (const auto& event : recommendations_.recommendEvents(student.id)) {
                recommendedEvents.push_back(toJson(event));
            }

            auto certificates = nlohmann::json::array();
            for (const auto& certificate : certificates_.forStudent(student.id)) {
                certificates.push_back(toJson(certificate));
            }

            response.set_content(nlohmann::json{
                {"profile", {{"id", student.id}, {"name", student.name},
                              {"email", student.email}, {"department", student.department}}},
                {"registrations", std::move(registrations)},
                {"notifications", std::move(notifications)},
                {"recommendedClubs", std::move(recommendedClubs)},
                {"recommendedEvents", std::move(recommendedEvents)},
                {"certificates", std::move(certificates)}
            }.dump(), "application/json; charset=utf-8");
            response.set_header("Cache-Control", "no-store");
        } catch (const std::exception& error) {
            setServiceError(response, error);
        }
    });

    server.Post("/api/student/registrations", [this](const httplib::Request& request, httplib::Response& response) {
        const auto session = requireRole(authentication_, request, response, "student");
        if (!session) return;
        const auto body = parseObject(request, response);
        if (!body) return;
        if (!body->contains("eventId") || !(*body)["eventId"].is_string()) {
            response.status = 400;
            response.set_content(R"({"error":"Event ID is required."})", "application/json; charset=utf-8");
            return;
        }

        try {
            const auto status = registrations_.registerStudent(session->subjectId,
                (*body)["eventId"].get<std::string>());
            response.status = 201;
            response.set_content(nlohmann::json{{"status", toString(status)}}.dump(),
                                 "application/json; charset=utf-8");
        } catch (const std::exception& error) {
            setServiceError(response, error);
        }
    });

    server.Post("/api/student/registrations/cancel",
        [this](const httplib::Request& request, httplib::Response& response) {
            const auto session = requireRole(authentication_, request, response, "student");
            if (!session) return;
            const auto body = parseObject(request, response);
            if (!body) return;
            if (!body->contains("eventId") || !(*body)["eventId"].is_string()) {
                response.status = 400;
                response.set_content(R"({"error":"Event ID is required."})", "application/json; charset=utf-8");
                return;
            }

            try {
                registrations_.cancel(session->subjectId, (*body)["eventId"].get<std::string>());
                response.set_content(R"({"cancelled":true})", "application/json; charset=utf-8");
            } catch (const std::exception& error) {
                setServiceError(response, error);
            }
        });

    server.Post("/api/student/notifications/read",
        [this](const httplib::Request& request, httplib::Response& response) {
            const auto session = requireRole(authentication_, request, response, "student");
            if (!session) return;
            notifications_.markAllRead(session->subjectId);
            response.set_content(R"({"updated":true})", "application/json; charset=utf-8");
        });

    server.Post("/api/student/feedback", [this](const httplib::Request& request, httplib::Response& response) {
        const auto session = requireRole(authentication_, request, response, "student");
        if (!session) return;
        const auto body = parseObject(request, response);
        if (!body) return;
        if (!body->contains("eventId") || !(*body)["eventId"].is_string() ||
            !body->contains("rating") || !(*body)["rating"].is_number_integer() ||
            !body->contains("comment") || !(*body)["comment"].is_string()) {
            response.status = 400;
            response.set_content(R"({"error":"Event, rating, and comment are required."})",
                                 "application/json; charset=utf-8");
            return;
        }

        try {
            feedback_.submit(session->subjectId, (*body)["eventId"].get<std::string>(),
                             (*body)["rating"].get<int>(), (*body)["comment"].get<std::string>());
            response.status = 201;
            response.set_content(R"({"saved":true})", "application/json; charset=utf-8");
        } catch (const std::exception& error) {
            setServiceError(response, error);
        }
    });

    server.Get("/api/coordinator/dashboard",
        [this](const httplib::Request& request, httplib::Response& response) {
            if (!requireRole(authentication_, request, response, "coordinator")) return;

            try {
                const auto students = students_.all();
                const auto events = events_.all();
                auto studentRecords = nlohmann::json::array();
                auto registrationCount = std::size_t{0};
                auto certificateCount = std::size_t{0};
                for (const auto& student : students) {
                    studentRecords.push_back({{"id", student.id}, {"name", student.name},
                                              {"department", student.department}});
                    registrationCount += registrations_.forStudent(student.id).size();
                    certificateCount += certificates_.forStudent(student.id).size();
                }

                auto eventRecords = nlohmann::json::array();
                auto attendanceCount = std::size_t{0};
                auto feedbackCount = std::size_t{0};
                for (const auto& event : events) {
                    eventRecords.push_back(toJson(event));
                    attendanceCount += attendance_.forEvent(event.id).size();
                    feedbackCount += feedback_.forEvent(event.id).size();
                }

                auto clubRecords = nlohmann::json::array();
                for (const auto& club : clubs_.all()) {
                    clubRecords.push_back(toJson(club));
                }

                response.set_content(nlohmann::json{
                    {"students", std::move(studentRecords)},
                    {"events", std::move(eventRecords)},
                    {"clubs", std::move(clubRecords)},
                    {"summary", {
                        {"students", students.size()},
                        {"events", events.size()},
                        {"clubs", clubs_.all().size()},
                        {"registrations", registrationCount},
                        {"attendance", attendanceCount},
                        {"certificates", certificateCount},
                        {"feedback", feedbackCount}
                    }}
                }.dump(), "application/json; charset=utf-8");
                response.set_header("Cache-Control", "no-store");
            } catch (const std::exception& error) {
                setServiceError(response, error);
            }
        });

    server.Post("/api/coordinator/events",
        [this](const httplib::Request& request, httplib::Response& response) {
            if (!requireRole(authentication_, request, response, "coordinator")) return;
            const auto body = parseObject(request, response);
            if (!body) return;
            for (const char* key : {"id", "clubId", "title", "description", "date", "time", "venue", "category"}) {
                if (!body->contains(key) || !(*body)[key].is_string()) {
                    response.status = 400;
                    response.set_content(nlohmann::json{{"error", std::string(key) + " is required."}}.dump(),
                                         "application/json; charset=utf-8");
                    return;
                }
            }
            if (!body->contains("capacity") || !(*body)["capacity"].is_number_integer()) {
                response.status = 400;
                response.set_content(R"({"error":"Capacity must be a number."})",
                                     "application/json; charset=utf-8");
                return;
            }

            try {
                Event event{(*body)["id"].get<std::string>(), (*body)["clubId"].get<std::string>(),
                            (*body)["title"].get<std::string>(), (*body)["description"].get<std::string>(),
                            (*body)["date"].get<std::string>(), (*body)["time"].get<std::string>(),
                            (*body)["venue"].get<std::string>(), (*body)["category"].get<std::string>(),
                            (*body)["capacity"].get<int>()};
                events_.add(event);
                recommendations_.rebuild();
                response.status = 201;
                response.set_content(toJson(event).dump(), "application/json; charset=utf-8");
            } catch (const std::exception& error) {
                setServiceError(response, error);
            }
        });

    server.Post("/api/coordinator/clubs",
        [this](const httplib::Request& request, httplib::Response& response) {
            if (!requireRole(authentication_, request, response, "coordinator")) return;
            const auto body = parseObject(request, response);
            if (!body) return;
            for (const char* key : {"id", "name", "description"}) {
                if (!body->contains(key) || !(*body)[key].is_string()) {
                    response.status = 400;
                    response.set_content(nlohmann::json{{"error", std::string(key) + " is required."}}.dump(),
                                         "application/json; charset=utf-8");
                    return;
                }
            }

            try {
                Club club{(*body)["id"].get<std::string>(), (*body)["name"].get<std::string>(),
                          (*body)["description"].get<std::string>(), {}, {}};
                if (body->contains("interests") && (*body)["interests"].is_array()) {
                    for (const auto& interest : (*body)["interests"]) {
                        if (interest.is_string()) club.interests.push_back(interest.get<std::string>());
                    }
                }
                clubs_.add(club);
                recommendations_.rebuild();
                response.status = 201;
                response.set_content(toJson(club).dump(), "application/json; charset=utf-8");
            } catch (const std::exception& error) {
                setServiceError(response, error);
            }
        });

    server.Post("/api/coordinator/clubs/members",
        [this](const httplib::Request& request, httplib::Response& response) {
            if (!requireRole(authentication_, request, response, "coordinator")) return;
            const auto body = parseObject(request, response);
            if (!body) return;
            if (!body->contains("clubId") || !(*body)["clubId"].is_string() ||
                !body->contains("studentId") || !(*body)["studentId"].is_string()) {
                response.status = 400;
                response.set_content(R"({"error":"Club ID and student ID are required."})",
                                     "application/json; charset=utf-8");
                return;
            }
            try {
                clubs_.addMember((*body)["clubId"].get<std::string>(),
                                 (*body)["studentId"].get<std::string>());
                recommendations_.rebuild();
                response.set_content(R"({"updated":true})", "application/json; charset=utf-8");
            } catch (const std::exception& error) {
                setServiceError(response, error);
            }
        });

    server.Post("/api/coordinator/attendance",
        [this](const httplib::Request& request, httplib::Response& response) {
            if (!requireRole(authentication_, request, response, "coordinator")) return;
            const auto body = parseObject(request, response);
            if (!body) return;
            if (!body->contains("studentId") || !(*body)["studentId"].is_string() ||
                !body->contains("eventId") || !(*body)["eventId"].is_string() ||
                !body->contains("present") || !(*body)["present"].is_boolean()) {
                response.status = 400;
                response.set_content(R"({"error":"Student, event, and attendance status are required."})",
                                     "application/json; charset=utf-8");
                return;
            }
            try {
                attendance_.mark((*body)["studentId"].get<std::string>(),
                                 (*body)["eventId"].get<std::string>(),
                                 (*body)["present"].get<bool>());
                response.set_content(R"({"saved":true})", "application/json; charset=utf-8");
            } catch (const std::exception& error) {
                setServiceError(response, error);
            }
        });

    server.Post("/api/coordinator/certificates",
        [this](const httplib::Request& request, httplib::Response& response) {
            if (!requireRole(authentication_, request, response, "coordinator")) return;
            const auto body = parseObject(request, response);
            if (!body) return;
            if (!body->contains("studentId") || !(*body)["studentId"].is_string() ||
                !body->contains("eventId") || !(*body)["eventId"].is_string()) {
                response.status = 400;
                response.set_content(R"({"error":"Student ID and event ID are required."})",
                                     "application/json; charset=utf-8");
                return;
            }
            try {
                const auto certificate = certificates_.issue((*body)["studentId"].get<std::string>(),
                                                             (*body)["eventId"].get<std::string>());
                response.set_content(nlohmann::json{
                    {"id", certificate.id}, {"title", certificate.title}, {"issuedAt", certificate.issuedAt}
                }.dump(), "application/json; charset=utf-8");
            } catch (const std::exception& error) {
                setServiceError(response, error);
            }
        });

    server.Get("/api/coordinator/feedback",
        [this](const httplib::Request& request, httplib::Response& response) {
            if (!requireRole(authentication_, request, response, "coordinator")) return;
            if (!request.has_param("eventId")) {
                response.status = 400;
                response.set_content(R"({"error":"Event ID is required."})", "application/json; charset=utf-8");
                return;
            }
            try {
                const auto eventId = request.get_param_value("eventId");
                auto feedback = nlohmann::json::array();
                for (const auto& item : feedback_.forEvent(eventId)) feedback.push_back(toJson(item));
                response.set_content(nlohmann::json{
                    {"averageRating", feedback_.averageRating(eventId)},
                    {"entries", std::move(feedback)}
                }.dump(), "application/json; charset=utf-8");
            } catch (const std::exception& error) {
                setServiceError(response, error);
            }
        });

    server.Get("/api/coordinator/report",
        [this](const httplib::Request& request, httplib::Response& response) {
            if (!requireRole(authentication_, request, response, "coordinator")) return;
            response.set_header("Content-Disposition", "attachment; filename=campus-connect-report.txt");
            response.set_header("Cache-Control", "no-store");
            response.set_content(reports_.summaryText(), "text/plain; charset=utf-8");
        });

    server.Post("/api/auth/login", [this](const httplib::Request& request, httplib::Response& response) {
        const auto body = nlohmann::json::parse(request.body, nullptr, false);
        if (body.is_discarded() || !body.is_object() ||
            !body.value("email", nlohmann::json{}).is_string() ||
            !body.value("password", nlohmann::json{}).is_string() ||
            !body.value("role", nlohmann::json{}).is_string()) {
            response.status = 400;
            response.set_content(R"({"error":"Email, password, and role are required."})",
                                 "application/json; charset=utf-8");
            return;
        }

        const auto session = authentication_.login(body["email"].get<std::string>(),
            body["password"].get<std::string>(), body["role"].get<std::string>());
        if (!session) {
            response.status = 401;
            response.set_content(R"({"error":"Email, password, or account type is incorrect."})",
                                 "application/json; charset=utf-8");
            return;
        }

        setSessionCookie(response, session->token);
        response.set_content(toJson(*session).dump(), "application/json; charset=utf-8");
    });

    server.Post("/api/auth/signup", [this](const httplib::Request& request, httplib::Response& response) {
        const auto body = nlohmann::json::parse(request.body, nullptr, false);
        const auto hasString = [&body](const char* key) {
            return body.is_object() && body.contains(key) && body[key].is_string();
        };
        if (body.is_discarded() || !hasString("email") || !hasString("password") || !hasString("role")) {
            response.status = 400;
            response.set_content(R"({"error":"Email, password, and account type are required."})",
                                 "application/json; charset=utf-8");
            return;
        }

        try {
            AuthSession session;
            const std::string role = body["role"].get<std::string>();
            if (role == "student" && hasString("studentId")) {
                session = authentication_.signUpStudent(body["studentId"].get<std::string>(),
                    body["email"].get<std::string>(), body["password"].get<std::string>());
            } else if (role == "coordinator" && hasString("displayName") && hasString("inviteCode")) {
                session = authentication_.signUpCoordinator(body["email"].get<std::string>(),
                    body["displayName"].get<std::string>(), body["password"].get<std::string>(),
                    body["inviteCode"].get<std::string>());
            } else {
                response.status = 400;
                response.set_content(R"({"error":"Required details are missing for this account type."})",
                                     "application/json; charset=utf-8");
                return;
            }

            setSessionCookie(response, session.token);
            response.set_content(toJson(session).dump(), "application/json; charset=utf-8");
        } catch (const std::exception& error) {
            response.status = 400;
            response.set_content(nlohmann::json{{"error", error.what()}}.dump(),
                                 "application/json; charset=utf-8");
        }
    });

    server.Get("/api/auth/session", [this](const httplib::Request& request, httplib::Response& response) {
        const auto session = authentication_.currentSession(sessionTokenFromCookie(request));
        if (session) {
            response.set_content(toJson(*session).dump(), "application/json; charset=utf-8");
        } else {
            response.set_content(R"({"authenticated":false})", "application/json; charset=utf-8");
        }
        response.set_header("Cache-Control", "no-store");
    });

    server.Post("/api/auth/logout", [this](const httplib::Request& request, httplib::Response& response) {
        authentication_.logout(sessionTokenFromCookie(request));
        response.set_header("Set-Cookie", "campus_session=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0");
        response.set_header("Cache-Control", "no-store");
        response.set_content(R"({"authenticated":false})", "application/json; charset=utf-8");
    });

    return server.listen("127.0.0.1", port);
}