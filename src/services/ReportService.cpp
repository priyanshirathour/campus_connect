#include "services/ReportService.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::string ReportService::summaryText() const {
	std::ostringstream report;
	report << "Campus Connect Report\n=====================\n"
		   << "Students: " << repository_.allStudents().size() << '\n'
		   << "Clubs: " << repository_.allClubs().size() << '\n'
		   << "Events: " << repository_.allEvents().size() << '\n'
		   << "Registrations: " << repository_.allRegistrations().size() << '\n'
		   << "Attendance: " << repository_.allAttendance().size() << '\n'
		   << "Certificates: " << repository_.allCertificates().size() << '\n'
		   << "Feedback: " << repository_.allFeedback().size() << '\n'
		   << "Notifications: " << repository_.allNotifications().size() << '\n';
	return report.str();
}

void ReportService::printSummary() const {
	std::cout << '\n' << summaryText();
}

void ReportService::exportSummary(const std::string& filePath) const {
	std::ofstream file(filePath);
	if (!file) {
		throw std::runtime_error("Could not write report: " + filePath);
	}
	file << summaryText();
}
