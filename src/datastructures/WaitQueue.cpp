#include "datastructures/WaitQueue.h"

#include <stdexcept>

void WaitQueue::push(const std::string& value) {
    // Avoid duplicate student IDs in a single event's waitlist.
    if (!contains(value)) queue_.push(value);
}

std::string WaitQueue::pop() {
    if (queue_.empty()) throw std::out_of_range("Cannot pop an empty WaitQueue");
    std::string value = queue_.front();
    queue_.pop();
    return value;
}

bool WaitQueue::empty() const noexcept { return queue_.empty(); }
std::size_t WaitQueue::size() const noexcept { return queue_.size(); }

bool WaitQueue::contains(const std::string& value) const {
    auto copy = queue_;
    while (!copy.empty()) {
        if (copy.front() == value) return true;
        copy.pop();
    }
    return false;
}

bool WaitQueue::remove(const std::string& value) {
    bool removed = false;
    std::queue<std::string> replacement;
    // Rebuilding the queue is necessary because std::queue has no erase operation;
    // copying the other IDs in sequence preserves their FIFO order.
    while (!queue_.empty()) {
        std::string current = queue_.front();
        queue_.pop();
        if (current == value && !removed) {
            removed = true;
        } else {
            replacement.push(current);
        }
    }
    queue_ = std::move(replacement);
    return removed;
}

std::vector<std::string> WaitQueue::toVector() const {
    std::vector<std::string> values;
    auto copy = queue_;
    while (!copy.empty()) {
        values.push_back(copy.front());
        copy.pop();
    }
    return values;
}

void WaitQueue::load(const std::vector<std::string>& values) {
    clear();
    for (const auto& value : values) push(value);
}

void WaitQueue::clear() noexcept {
    while (!queue_.empty()) queue_.pop();
}
