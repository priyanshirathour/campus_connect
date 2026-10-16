#include "services/StudentService.h"
#include "exceptions/CampusConnectException.h"
StudentService::StudentService(Repository& repository) : repository_(repository) { refresh(); }
void StudentService::refresh() { studentsById_.clear(); for (const auto& s : repository_.allStudents()) studentsById_[s.id] = s; }
const Student& StudentService::get(const std::string& id) const { auto it=studentsById_.find(id); if(it==studentsById_.end()) throw StudentNotFoundException(id); return it->second; }
std::vector<Student> StudentService::all() const { std::vector<Student> v; for(const auto& item:studentsById_) v.push_back(item.second); return v; }
void StudentService::add(const Student& s) { if(s.id.empty()||s.name.empty()) throw InvalidInputException("Student ID and name are required."); if(studentsById_.count(s.id)) throw InvalidInputException("Student already exists: "+s.id); repository_.save(s); studentsById_[s.id]=s; }
