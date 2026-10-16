#pragma once
#include "database/Repository.h"
#include <unordered_map>
class StudentService {
public:
    explicit StudentService(Repository& repository);
    const Student& get(const std::string& id) const;
    std::vector<Student> all() const;
    void add(const Student& student);
private:
    Repository& repository_;
    std::unordered_map<std::string, Student> studentsById_;
    void refresh();
};
