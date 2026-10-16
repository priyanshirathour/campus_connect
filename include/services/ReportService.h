#pragma once
#include "database/Repository.h"
#include <string>
class ReportService {
public:
    explicit ReportService(Repository& repository) : repository_(repository) {}
    std::string summaryText() const;
    void printSummary() const;
    void exportSummary(const std::string& filePath) const;
private:
    Repository& repository_;
};
