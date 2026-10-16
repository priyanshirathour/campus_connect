#pragma once
#include "database/Repository.h"
#include "datastructures/WaitQueue.h"
#include <unordered_map>
class NotificationService {
public:
    explicit NotificationService(Repository& repository);
    void notify(const std::string& studentId, const std::string& message);
    std::vector<Notification> forStudent(const std::string& studentId) const;
    void markAllRead(const std::string& studentId);
private:
    Repository& repository_;
    std::unordered_map<std::string, Notification> notificationsById_;
    std::unordered_map<std::string, WaitQueue> unreadQueuesByStudent_;
    void refresh();
};
