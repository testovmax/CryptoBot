#ifndef CRYPTOBOT_HPP
#define CRYPTOBOT_HPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <mutex>
#include <atomic>
#include "CurrencyManager.hpp"
#include "UserManager.hpp"
#include "TelegramHandler.hpp"

struct Alert {
    long userId;
    std::string crypto;
    double targetPrice;
    bool isAbove;
    time_t createdAt;
    
    Alert(long uid = 0, const std::string& c = "", double price = 0, bool above = true);
    std::string getDescription() const;
    bool shouldTrigger(double currentPrice) const;
};

class CryptoBot {
private:
    TelegramHandler telegram;
    CurrencyManager currencies;
    UserManager users;
    std::vector<Alert> alerts;
    mutable std::mutex alertsMutex;
    
    std::atomic<bool> running{true};
    
    static std::atomic<bool> stopRequested;
    static void handleSignal(int signal);
    
public:
    CryptoBot(const std::string& token);
    
    // Основные методы
    void run();
    void stop() { running = false; }
    
    // Обработка команд
    std::string processCommand(long userId, const std::string& command, 
                              const std::string& username = "");
    
    // Оповещения
    void addAlert(long userId, const std::string& crypto, double price, bool isAbove);
    void checkAlerts();
    
    // Форматирование
    std::string formatAlerts(long userId) const;
    
private:
    void processMessages();
    void autoTasks();
};

#endif