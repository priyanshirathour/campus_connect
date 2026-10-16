#include "services/NotificationService.h"
#include "utils/Utils.h"
NotificationService::NotificationService(Repository &repository) : repository_(repository) { refresh(); }
void NotificationService::refresh()
{
    notificationsById_.clear();
    unreadQueuesByStudent_.clear();
    for (const auto &n : repository_.allNotifications())
    {
        notificationsById_[n.id] = n;
        if (!n.read)
            unreadQueuesByStudent_[n.studentId].push(n.id);
    }
}
void NotificationService::notify(const std::string &studentId, const std::string &message)
{
    Notification n{makeId("N"), studentId, message, nowTimestamp(), false};
    repository_.save(n);
    notificationsById_[n.id] = n;
    unreadQueuesByStudent_[studentId].push(n.id);
}
std::vector<Notification> NotificationService::forStudent(const std::string &studentId) const
{
    std::vector<Notification> result;
    auto it = unreadQueuesByStudent_.find(studentId);
    if (it == unreadQueuesByStudent_.end())
        return result;
    for (const auto &id : it->second.toVector())
    {
        auto n = notificationsById_.find(id);
        if (n != notificationsById_.end())
            result.push_back(n->second);
    }
    return result;
}
void NotificationService::markAllRead(const std::string &studentId)
{
    auto it = unreadQueuesByStudent_.find(studentId);
    if (it == unreadQueuesByStudent_.end())
        return;
    for (const auto &id : it->second.toVector())
    {
        auto n = notificationsById_.find(id);
        if (n != notificationsById_.end())
        {
            n->second.read = true;
            repository_.save(n->second);
        }
    }
    it->second.clear();
}
