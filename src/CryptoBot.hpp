#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "CurrencyManager.hpp"
#include "UserManager.hpp"
#include "TelegramHandler.hpp"
#include "HistoryLogger.hpp"

struct Alert {
    long userId;
    std::string crypto;
    double targetPrice;
    bool isAbove;
    time_t createdAt;

    Alert(long uid, const std::string& c, double price, bool above);

    bool shouldTrigger(double currentPrice) const {
        return isAbove ? currentPrice >= targetPrice : currentPrice <= targetPrice;
    }
};

class CryptoBot {
private:
    std::string botToken;
    TelegramHandler telegram;
    CurrencyManager currencies;
    UserManager users;
    HistoryLogger logger;

    std::vector<Alert> alerts;
    mutable std::mutex alertsMutex;

    static std::atomic<bool> stopRequested;
    static std::mutex coutMutex;

    void processMessages();
    void autoTasks();
    void checkAlerts();
    void addAlert(long userId, const std::string& crypto, double price, bool isAbove);
    std::string formatAlerts(long userId) const;
    
    std::string processCommand(long userId, const std::string& command, const std::string& username);


public:
    static std::string readBotToken();
    CryptoBot(const std::string& token);
    void run();
    static void handleSignal(int signal);
};