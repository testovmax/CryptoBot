#pragma once

#include <string>
#include <vector>
#include <mutex>

class HistoryLogger {
private:
    std::vector<std::string> history;
    size_t maxSize;
    mutable std::mutex mutex;

public:
    explicit HistoryLogger(size_t maxEntries = 1000);
    void add(const std::string& entry);
    std::vector<std::string> getRecent(size_t count) const;
    size_t size() const;
};