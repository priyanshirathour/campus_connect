#include "services/CertificateService.h"
#include "exceptions/CampusConnectException.h"
#include "utils/Utils.h"

CertificateService::CertificateService(Repository& repository, StudentService& students,
									   EventService& events, AttendanceService& attendance)
	: repository_(repository), students_(students), events_(events), attendance_(attendance) {}

Certificate CertificateService::issue(const std::string& studentId, const std::string& eventId) {
	students_.get(studentId);
	const Event event = events_.get(eventId);
	if (!attendance_.isPresent(studentId, eventId)) {
		throw CampusConnectException("Certificate cannot be issued until attendance is marked PRESENT.");
	}

	const std::string certificateId = "CERT-" + studentId + "-" + eventId;
	for (const auto& certificate : repository_.allCertificates()) {
		if (certificate.id == certificateId) {
			return certificate;
		}
	}

	Certificate certificate{certificateId, studentId, eventId, event.title, nowTimestamp()};
	repository_.save(certificate);
	return certificate;
}

std::vector<Certificate> CertificateService::forStudent(const std::string& studentId) const {
	std::vector<Certificate> certificates;
	for (const auto& certificate : repository_.allCertificates()) {
		if (certificate.studentId == studentId) {
			certificates.push_back(certificate);
		}
	}
	return certificates;
}
