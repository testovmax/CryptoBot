#include "CryptoBot.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <chrono>

Alert::Alert(long uid, const std::string& c, double price, bool above)
    : userId(uid), crypto(c), targetPrice(price), isAbove(above), 
      createdAt(time(nullptr)) {}

std::string Alert::getDescription() const {
    std::stringstream ss;
    ss << crypto << " " << (isAbove ? "выше" : "ниже") 
       << " $" << std::fixed << std::setprecision(2) << targetPrice;
    return ss.str();
}

bool Alert::shouldTrigger(double currentPrice) const {
    if (isAbove) {
        return currentPrice >= targetPrice;
    } else {
        return currentPrice <= targetPrice;
    }
}

CryptoBot::CryptoBot(const std::string& token) : telegram(token) {
    std::cout << "🤖 КриптоБот создан\n";
    
    if (telegram.testConnection()) {
        std::cout << "✅ Подключение к Telegram успешно\n";
    } else {
        std::cout << "⚠️  Нет подключения к Telegram\n";
    }
}

void CryptoBot::run() {
    std::cout << "🚀 Запуск бота...\n";
    
    // Инициализация
    currencies.updatePrices();
    
    std::cout << "Бот активен. Ожидание сообщений из Telegram...\n";
    
    while (running) {
        // 1. Обработка сообщений из Telegram
        processMessages();
        
        // 2. Автоматические задачи (обновление цен, проверка алертов)
        autoTasks();
        
        // 3. Минимальная пауза для CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "\nЗавершение работы бота...\n";
}

void CryptoBot::processMessages() {
    auto messages = telegram.getMessages();
    
    for (const auto& msg : messages) {
        try {
            std::string response = processCommand(msg.chatId, msg.text, msg.username);
            telegram.sendMessage(msg.chatId, response);
        } catch (const std::exception& e) {
            std::cerr << "❌ Ошибка: " << e.what() << std::endl;
            telegram.sendMessage(msg.chatId, "❌ Произошла ошибка");
        }
    }
}

void CryptoBot::autoTasks() {
    static time_t lastPriceUpdate = time(nullptr);
    static time_t lastAlertCheck = time(nullptr);
    time_t now = time(nullptr);
    
    // Обновление цен каждую минуту
    if (difftime(now, lastPriceUpdate) >= 60) {
        currencies.updatePrices();
        lastPriceUpdate = now;
    }
    
    // Проверка оповещений каждые 30 секунд
    if (difftime(now, lastAlertCheck) >= 30) {
        checkAlerts();
        lastAlertCheck = now;
    }
}

std::string CryptoBot::processCommand(long userId, const std::string& command, 
                                     const std::string& username) {
    // Обновляем пользователя
    if (!users.userExists(userId)) {
        users.createUser(userId, username);
    }
    users.recordCommand(userId);
    
    // Извлекаем команду и аргументы
    std::string cmd = command;
    std::vector<std::string> args;
    
    if (cmd[0] == '/') cmd = cmd.substr(1);
    
    std::stringstream ss(cmd);
    std::string token;
    while (std::getline(ss, token, ' ')) {
        if (!token.empty()) args.push_back(token);
    }
    
    if (args.empty()) return "Пустая команда";
    
    std::string commandName = args[0];
    args.erase(args.begin());
    
    // Приводим к нижнему регистру
    std::transform(commandName.begin(), commandName.end(), 
                   commandName.begin(), ::tolower);
    
    // Обработка команд
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
        if (args.empty()) return "Укажите символ валюты: /price BTC";
        
        std::string symbol = args[0];
        if (!currencies.currencyExists(symbol)) {
            return "❌ Валюта не найдена: " + symbol;
        }
        
        users.addFavorite(userId, symbol);
        return currencies.formatCurrencyInfo(symbol);
    }
    else if (commandName == "convert") {
        if (args.size() < 4) {
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
        
        if (amount <= 0) return "❌ Сумма должна быть положительной";
        
        return currencies.formatConversion(amount, from, to);
    }
    else if (commandName == "alert") {
        if (args.size() < 3) {
            return "Формат: /alert [символ] [above/below] [цена]\nПример: /alert BTC above 50000";
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
            price = std::stod(priceStr);
        } catch (...) {
            return "❌ Неверная цена";
        }
        
        if (price <= 0) return "❌ Цена должна быть положительной";
        
        addAlert(userId, symbol, price, condition == "above");
        users.addFavorite(userId, symbol);
        
        Currency currency = currencies.getCurrency(symbol);
        
        std::stringstream response;
        response << "✅ Оповещение создано!\n\n";
        response << symbol << " " << (condition == "above" ? "выше" : "ниже") 
                 << " $" << std::fixed << std::setprecision(2) << price << "\n";
        response << "Текущая цена: $" << std::fixed << std::setprecision(2) << currency.price;
        
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
    
    // Удаляем старый алерт
    alerts.erase(std::remove_if(alerts.begin(), alerts.end(),
        [userId, crypto](const Alert& a) {
            return a.userId == userId && a.crypto == crypto;
        }), alerts.end());
    
    alerts.emplace_back(userId, crypto, price, isAbove);
    
    std::cout << "🔔 Создано оповещение: " << userId 
              << " - " << crypto << " " << (isAbove ? ">" : "<") 
              << " $" << price << std::endl;
}

void CryptoBot::checkAlerts() {
    std::lock_guard<std::mutex> lock(alertsMutex);
    
    if (alerts.empty()) {
        return;
    }
    
    std::vector<Alert> triggeredAlerts;
    
    for (auto it = alerts.begin(); it != alerts.end();) {
        Currency currency = currencies.getCurrency(it->crypto);
        if (currency.name == "Unknown") {
            ++it;
            continue;
        }
        
        if (it->shouldTrigger(currency.price)) {
            triggeredAlerts.push_back(*it);
            
            std::stringstream ss;
            ss << "🔔 ОПОВЕЩЕНИЕ СРАБОТАЛО!\n\n";
            ss << it->getDescription() << "\n\n";
            ss << "Текущая цена: $" << std::fixed << std::setprecision(2) << currency.price << "\n";
            ss << "Целевая цена: $" << std::fixed << std::setprecision(2) << it->targetPrice << "\n\n";
            
            double difference = std::abs(currency.price - it->targetPrice);
            ss << "Разница: $" << std::fixed << std::setprecision(2) << difference;
            
            telegram.sendMessage(it->userId, ss.str());
            
            it = alerts.erase(it);
        } else {
            ++it;
        }
    }
    
    if (!triggeredAlerts.empty()) {
        std::cout << "⚠️  Сработало " << triggeredAlerts.size() << " оповещений\n";
    }
}

std::string CryptoBot::formatAlerts(long userId) const {
    std::lock_guard<std::mutex> lock(alertsMutex);
    
    std::vector<Alert> userAlerts;
    for (const auto& alert : alerts) {
        if (alert.userId == userId) {
            userAlerts.push_back(alert);
        }
    }
    
    if (userAlerts.empty()) {
        return "📭 У вас нет активных оповещений.\n\n"
               "Создайте оповещение:\n"
               "/alert BTC above 50000\n"
               "/alert ETH below 3000";
    }
    
    std::stringstream ss;
    ss << "📋 Ваши активные оповещения (" << userAlerts.size() << "):\n\n";
    
    int index = 1;
    for (const auto& alert : userAlerts) {
        Currency currency = currencies.getCurrency(alert.crypto);
        
        ss << index << ". " << alert.getDescription() << "\n";
        ss << "   Текущая цена: $" << std::fixed << std::setprecision(2) << currency.price << "\n";
        
        double difference = alert.isAbove ? 
            (alert.targetPrice - currency.price) : 
            (currency.price - alert.targetPrice);
        
        if (difference > 0) {
            ss << "   Осталось: $" << std::fixed << std::setprecision(2) << difference;
            
            double percent = (difference / alert.targetPrice) * 100.0;
            if (percent > 0.1) {
                ss << " (" << std::fixed << std::setprecision(1) << percent << "%)";
            }
        } else {
            ss << "   ⚠️ Цель почти достигнута!";
        }
        
        ss << "\n\n";
        index++;
    }
    
    return ss.str();
}
