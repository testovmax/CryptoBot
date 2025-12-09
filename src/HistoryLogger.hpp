// HistoryLogger.hpp
#ifndef HISTORY_LOGGER_HPP
#define HISTORY_LOGGER_HPP

#include <string>
#include <vector>
#include <map>
#include <ctime>

struct HistoryEntry {
    time_t timestamp;
    long userId;
    std::string command;
    std::string details;
    std::string result;
    
    HistoryEntry(long uid, const std::string& cmd, const std::string& det = "", 
                 const std::string& res = "");
    std::string format() const;
};

class HistoryLogger {
private:
    std::vector<HistoryEntry> entries;
    size_t maxEntries;
    
    void trimHistory();
    
public:
    HistoryLogger(size_t max = 1000);
    
    // Логирование
    void logCommand(long userId, const std::string& command, 
                    const std::string& details = "", const std::string& result = "");
    
    // Получение истории
    std::vector<HistoryEntry> getUserHistory(long userId, int limit = 50) const;
    std::vector<HistoryEntry> getRecentHistory(int limit = 100) const;
    std::vector<HistoryEntry> getCommandHistory(const std::string& command, 
                                                int limit = 50) const;
    
    // Статистика
    int getTotalCommands() const;
    int getUserCommandCount(long userId) const;
    std::map<std::string, int> getCommandStats() const;
    std::map<long, int> getUserStats() const;
    
    // Форматирование
    std::string formatUserHistory(long userId, int limit = 10) const;
    std::string formatRecentActivity(int limit = 20) const;
    std::string formatStats() const;
    
    // Очистка
    void clearOldEntries(int days = 30);
    void clearAll();
};

#endif