#include "HistoryLogger.hpp"
#include <algorithm>

HistoryLogger::HistoryLogger(size_t maxEntries) : maxSize(maxEntries) {}

void HistoryLogger::add(const std::string& entry) {
    std::lock_guard<std::mutex> lock(mutex);
    history.push_back(entry);
    if (history.size() > maxSize) {
        history.erase(history.begin());
    }
}

std::vector<std::string> HistoryLogger::getRecent(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex);
    size_t n = std::min(count, history.size());
    return std::vector<std::string>(history.end() - n, history.end());
}

size_t HistoryLogger::size() const {
    std::lock_guard<std::mutex> lock(mutex);
    return history.size();
}