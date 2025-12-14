// src/CryptoBot.cpp
#include "CryptoBot.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <csignal>
#include <fstream>

std::atomic<bool> CryptoBot::stopRequested{false};
std::mutex CryptoBot::coutMutex;

Alert::Alert(long uid, const std::string& c, double price, bool above)
    : userId(uid), crypto(c), targetPrice(price), isAbove(above),
      createdAt(time(nullptr)) {}

CryptoBot::CryptoBot(const std::string& token)
    : botToken(token), 
      telegram(token), 
      currencies(), 
      users(), 
      logger(1000) {}

void CryptoBot::run() {
    std::cout << "🚀 Запуск бота...\n";
    std::cout << "@CryptoLabuba_bot работает\n\n";

    while (!stopRequested) {
        processMessages();
        autoTasks();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void CryptoBot::processMessages() {
    std::vector<Message> messages = telegram.getMessages();
    for (const auto& msg : messages) {
        std::string response = processCommand(msg.chatId, msg.text, msg.username);
        telegram.sendMessage(msg.chatId, response);
    }
}

std::string CryptoBot::processCommand(long userId, const std::string& command, const std::string& username) {
    if (!users.userExists(userId)) {
        users.createUser(userId, username.empty() ? "пользователь" : username);
    }
    users.recordCommand(userId);

    std::string cmd = command;
    std::vector<std::string> args;

    if (cmd[0] == '/') {
        cmd = cmd.substr(1);
    }

    std::stringstream ss(cmd);
    std::string token;
    while (std::getline(ss, token, ' ')) {
        if (!token.empty()) {
            args.push_back(token);
        }
    }

    if (args.empty()) {
        return "Пустая команда";
    }

    std::string commandName = args[0];
    args.erase(args.begin());
    std::transform(commandName.begin(), commandName.end(), commandName.begin(), ::tolower);

    if (commandName == "start") {
        return TelegramHandler::getWelcomeText(username.empty() ? "пользователь" : username);
    }
    else if (commandName == "help") {
        return TelegramHandler::getHelpText();
    }
    else if (commandName == "list") {
        return currencies.formatCurrencyList();
    }
    else if (commandName == "price") {
        if (args.empty()) {
            return "Укажите символ валюты: /price BTC";
        }
        std::string symbol = args[0];
        if (!currencies.currencyExists(symbol)) {
            return "❌ Валюта не найдена: " + symbol;
        }
        users.addFavorite(userId, symbol);
        return currencies.formatCurrencyInfo(symbol);
    }
    else if (commandName == "convert") {
        if (args.size() < 4 || args[2] != "to") {
            return "Формат: /convert [сумма] [из] to [в]\nПример: /convert 1 BTC to USD";
        }
        std::string amountStr = args[0];
        std::string from = args[1];
        std::string to = args[3];
        double amount;
        try {
            amount = std::stod(amountStr);
        } catch (...) {
            return "❌ Неверная сумма";
        }
        if (amount <= 0) {
            return "❌ Сумма должна быть положительной";
        }
        return currencies.formatConversion(amount, from, to);
    }
    else if (commandName == "alert") {
        if (args.size() < 3) {
            return "❌ Недостаточно аргументов\nФормат: /alert [символ] [above/below] [цена]";
        }
        std::string symbol = args[0];
        std::string condition = args[1];
        std::string priceStr = args[2];

        std::transform(condition.begin(), condition.end(), condition.begin(), ::tolower);
        if (condition != "above" && condition != "below") {
            return "❌ Условие должно быть 'above' или 'below'";
        }

        if (!currencies.currencyExists(symbol)) {
            return "❌ Валюта не найдена: " + symbol;
        }

        double price;
        try {
            size_t pos;
            price = std::stod(priceStr, &pos);
            if (pos != priceStr.length()) {
                return "❌ Неверный формат числа";
            }
            if (price <= 0) {
                return "❌ Цена должна быть положительной";
            }
        } catch (...) {
            return "❌ Не удалось распознать цену";
        }

        addAlert(userId, symbol, price, condition == "above");
        users.addFavorite(userId, symbol);

        Currency currency = currencies.getCurrency(symbol);
        std::stringstream response;
        response << std::fixed << std::setprecision(2)
                 << "✅ Оповещение создано!\n\n"
                 << symbol << " " << (condition == "above" ? "выше" : "ниже") 
                 << " USD " << price << "\n"
                 << "Текущая цена: USD " << currency.price;
        return response.str();
    }
    else if (commandName == "myalerts") {
        return formatAlerts(userId);
    }
    else if (commandName == "mystats") {
        return users.getUserStats(userId);
    }
    else if (commandName == "stats") {
        std::stringstream ss;
        ss << users.getStats() << "\n";
        ss << "Валют отслеживается: " << currencies.getCurrencyCount() << "\n";
        ss << "Активных оповещений: " << alerts.size();
        return ss.str();
    }
    else {
        return "❌ Неизвестная команда: /" + commandName + 
               "\nИспользуйте /help для списка команд";
    }
}

void CryptoBot::addAlert(long userId, const std::string& crypto, double price, bool isAbove) {
    std::lock_guard<std::mutex> lock(alertsMutex);
    alerts.erase(std::remove_if(alerts.begin(), alerts.end(),
        [userId, crypto](const Alert& a) {
            return a.userId == userId && a.crypto == crypto;
        }), alerts.end());
    alerts.emplace_back(userId, crypto, price, isAbove);
}

std::string CryptoBot::formatAlerts(long userId) const {
    std::lock_guard<std::mutex> lock(alertsMutex);
    std::stringstream ss;
    bool hasAlerts = false;
    for (const auto& alert : alerts) {
        if (alert.userId == userId) {
            if (!hasAlerts) {
                ss << "🔔 Ваши активные оповещения:\n\n";
                hasAlerts = true;
            }
            std::string condition = alert.isAbove ? "выше" : "ниже";
            Currency currency = currencies.getCurrency(alert.crypto);
            ss << std::fixed << std::setprecision(2)
               << alert.crypto << " " << condition << " USD " << alert.targetPrice << "\n"
               << "Текущая цена: USD " << currency.price << "\n\n";
        }
    }
    if (!hasAlerts) {
        ss << "🔕 У вас нет активных оповещений";
    }
    return ss.str();
}

void CryptoBot::checkAlerts() {
    std::vector<Alert> triggered;
    {
        std::lock_guard<std::mutex> lock(alertsMutex);
        for (auto it = alerts.begin(); it != alerts.end();) {
            Currency currency = currencies.getCurrency(it->crypto);
            if (currency.name != "Unknown" && it->shouldTrigger(currency.price)) {
                triggered.push_back(*it);
                it = alerts.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (const auto& alert : triggered) {
        Currency currency = currencies.getCurrency(alert.crypto);
        std::stringstream message;
        message << std::fixed << std::setprecision(2)
                << "🔔 ОПОВЕЩЕНИЕ СРАБОТАЛО!\n\n"
                << alert.crypto << " " << (alert.isAbove ? "выше" : "ниже") 
                << " USD " << alert.targetPrice << "\n"
                << "Текущая цена: USD " << currency.price << "\n"
                << "Целевая цена: USD " << alert.targetPrice << "\n"
                << "Разница: USD " 
                << (alert.isAbove ? currency.price - alert.targetPrice 
                                  : alert.targetPrice - currency.price);
        telegram.sendMessage(alert.userId, message.str());
    }
}

void CryptoBot::autoTasks() {
    currencies.updatePrices();
    checkAlerts();
}

void CryptoBot::handleSignal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n🛑 Получен сигнал остановки. Завершаем работу...\n";
        stopRequested = true;
    }
}

std::string CryptoBot::readBotToken() {
    std::ifstream file("bot_token.txt");
    if (file.is_open()) {
        std::string token;
        std::getline(file, token);
        file.close();
        if (!token.empty()) {
            return token;
        }
    }
    std::cout << "Введите токен бота: ";
    std::string token;
    std::getline(std::cin, token);
    if (!token.empty()) {
        std::ofstream out("bot_token.txt");
        if (out.is_open()) {
            out << token;
            out.close();
        }
    }
    return token;
}
