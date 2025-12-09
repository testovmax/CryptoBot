// HistoryLogger.cpp
#include "HistoryLogger.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <map>
#include <iostream>

HistoryEntry::HistoryEntry(long uid, const std::string& cmd, 
                           const std::string& det, const std::string& res)
    : timestamp(time(nullptr)), userId(uid), command(cmd), details(det), result(res) {}

std::string HistoryEntry::format() const {
    std::stringstream ss;
    
    // Форматирование времени
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", std::localtime(&timestamp));
    
    ss << "[" << timeStr << "] ";
    ss << "User " << userId << ": " << command;
    
    if (!details.empty()) {
        ss << " (" << details << ")";
    }
    
    if (!result.empty() && result.length() < 100) {
        ss << " -> " << result.substr(0, 50);
        if (result.length() > 50) ss << "...";
    }
    
    return ss.str();
}

HistoryLogger::HistoryLogger(size_t max) : maxEntries(max) {}

void HistoryLogger::trimHistory() {
    if (entries.size() > maxEntries) {
        // Удаляем самые старые записи
        size_t toRemove = entries.size() - maxEntries;
        entries.erase(entries.begin(), entries.begin() + toRemove);
    }
}

void HistoryLogger::logCommand(long userId, const std::string& command, 
                               const std::string& details, const std::string& result) {
    entries.emplace_back(userId, command, details, result);
    trimHistory();
}

std::vector<HistoryEntry> HistoryLogger::getUserHistory(long userId, int limit) const {
    std::vector<HistoryEntry> result;
    
    // Идем с конца (самые новые записи сначала)
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
        if (it->userId == userId) {
            result.push_back(*it);
            if (result.size() >= static_cast<size_t>(limit)) {
                break;
            }
        }
    }
    
    return result;
}

std::vector<HistoryEntry> HistoryLogger::getRecentHistory(int limit) const {
    std::vector<HistoryEntry> result;
    
    int count = 0;
    for (auto it = entries.rbegin(); it != entries.rend() && count < limit; ++it, ++count) {
        result.push_back(*it);
    }
    
    return result;
}

std::vector<HistoryEntry> HistoryLogger::getCommandHistory(const std::string& command, 
                                                           int limit) const {
    std::vector<HistoryEntry> result;
    
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
        if (it->command == command) {
            result.push_back(*it);
            if (result.size() >= static_cast<size_t>(limit)) {
                break;
            }
        }
    }
    
    return result;
}

int HistoryLogger::getTotalCommands() const {
    return entries.size();
}

int HistoryLogger::getUserCommandCount(long userId) const {
    int count = 0;
    for (const auto& entry : entries) {
        if (entry.userId == userId) {
            count++;
        }
    }
    return count;
}

std::map<std::string, int> HistoryLogger::getCommandStats() const {
    std::map<std::string, int> stats;
    
    for (const auto& entry : entries) {
        stats[entry.command]++;
    }
    
    return stats;
}

std::map<long, int> HistoryLogger::getUserStats() const {
    std::map<long, int> stats;
    
    for (const auto& entry : entries) {
        stats[entry.userId]++;
    }
    
    return stats;
}

std::string HistoryLogger::formatUserHistory(long userId, int limit) const {
    auto userHistory = getUserHistory(userId, limit);
    
    if (userHistory.empty()) {
        return "📭 История команд пуста.";
    }
    
    std::stringstream ss;
    ss << "📜 История ваших команд (" << userHistory.size() << " последних):\n\n";
    
    for (size_t i = 0; i < userHistory.size(); i++) {
        ss << (i + 1) << ". " << userHistory[i].format() << "\n";
    }
    
    ss << "\nВсего команд: " << getUserCommandCount(userId);
    
    return ss.str();
}

std::string HistoryLogger::formatRecentActivity(int limit) const {
    auto recent = getRecentHistory(limit);
    
    if (recent.empty()) {
        return "Нет недавней активности.";
    }
    
    std::stringstream ss;
    ss << "🔄 Недавняя активность:\n\n";
    
    for (size_t i = 0; i < recent.size(); i++) {
        ss << (i + 1) << ". " << recent[i].format() << "\n";
    }
    
    return ss.str();
}

std::string HistoryLogger::formatStats() const {
    auto cmdStats = getCommandStats();
    auto userStats = getUserStats();
    
    std::stringstream ss;
    ss << "📊 Статистика логов:\n\n";
    ss << "Всего записей: " << getTotalCommands() << "\n";
    ss << "Уникальных пользователей: " << userStats.size() << "\n";
    ss << "Уникальных команд: " << cmdStats.size() << "\n\n";
    
    ss << "🏆 Топ-5 команд:\n";
    std::vector<std::pair<std::string, int>> cmdVec(cmdStats.begin(), cmdStats.end());
    std::sort(cmdVec.begin(), cmdVec.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (int i = 0; i < std::min(5, (int)cmdVec.size()); i++) {
        ss << (i + 1) << ". " << cmdVec[i].first << ": " << cmdVec[i].second << " раз\n";
    }
    
    ss << "\n🏆 Топ-5 пользователей:\n";
    std::vector<std::pair<long, int>> userVec(userStats.begin(), userStats.end());
    std::sort(userVec.begin(), userVec.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (int i = 0; i < std::min(5, (int)userVec.size()); i++) {
        ss << (i + 1) << ". User " << userVec[i].first << ": " << userVec[i].second << " команд\n";
    }
    
    return ss.str();
}

void HistoryLogger::clearOldEntries(int days) {
    time_t cutoff = time(nullptr) - (days * 24 * 60 * 60);
    
    auto it = std::remove_if(entries.begin(), entries.end(),
        [cutoff](const HistoryEntry& entry) {
            return entry.timestamp < cutoff;
        });
    
    int removed = std::distance(it, entries.end());
    entries.erase(it, entries.end());
    
    if (removed > 0) {
        std::cout << "Удалено " << removed << " старых записей логов (старше " << days << " дней)" << std::endl;
    }
}

void HistoryLogger::clearAll() {
    entries.clear();
    std::cout << "Все записи логов очищены" << std::endl;
}