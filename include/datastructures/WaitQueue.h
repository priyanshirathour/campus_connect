#pragma once

#include <cstddef>
#include <queue>
#include <string>
#include <vector>

// FIFO queue for the fair first-come-first-served waitlist.
// Extra helper methods intentionally wrap std::queue; callers should not pretend
// that the STL queue itself has contains(), toVector(), or load().
class WaitQueue {
public:
    void push(const std::string& value);
    std::string pop();
    bool empty() const noexcept;
    std::size_t size() const noexcept;
    bool contains(const std::string& value) const;
    bool remove(const std::string& value);
    std::vector<std::string> toVector() const;
    void load(const std::vector<std::string>& values);
    void clear() noexcept;

private:
    std::queue<std::string> queue_;
};
