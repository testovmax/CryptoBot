#include "TelegramHandler.hpp"
#include <iostream>
#include <sstream>
#include <memory>
#include <array>
#include <algorithm>
#include <json.hpp>
#include <cctype>
#include <cstdio>

using json = nlohmann::json;

std::string TelegramHandler::execCommand(const std::string& cmd) const {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    
    if (!pipe) return "";
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return result;
}

std::string TelegramHandler::sendRequest(const std::string& method, 
                                        const std::string& data) const {
    std::string url = "https://api.telegram.org/bot" + botToken + "/" + method;
    std::string cmd = "curl -s -X POST \"" + url + "\"";
    
    if (!data.empty()) {
        cmd += " -d \"" + data + "\"";
    }
    
    return execCommand(cmd);
}

TelegramHandler::TelegramHandler(const std::string& token) 
    : botToken(token), lastUpdateId(0) {
    
    std::cout << "🤖 Telegram обработчик инициализирован\n";
}

std::vector<TelegramMessage> TelegramHandler::getMessages() {
    std::vector<TelegramMessage> messages;
    
    std::string url = "https://api.telegram.org/bot" + botToken + "/getUpdates";
    std::string cmd = "curl -s \"" + url + "\"";
    
    if (lastUpdateId > 0) {
        cmd += "?offset=" + std::to_string(lastUpdateId + 1);
    } else {
        cmd += "?offset=0";
    }
    
    cmd += "&timeout=2";
    
    std::string response = execCommand(cmd);
    
    if (response.empty()) return messages;
    
    try {
        json j = json::parse(response);
        
        if (!j["ok"].get<bool>()) return messages;
        
        // Обрабатываем обновления и обновляем lastUpdateId для ВСЕХ обновлений
        long maxUpdateId = lastUpdateId;
        
        for (const auto& update : j["result"]) {
            long updateId = update["update_id"].get<long>();
            
            // Пропускаем уже обработанные обновления
            if (updateId <= lastUpdateId) {
                continue;
            }
            
            // Обновляем maxUpdateId для ВСЕХ обновлений (даже без текста)
            maxUpdateId = std::max(maxUpdateId, updateId);
            
            // Обрабатываем только текстовые сообщения
            if (!update.contains("message") || !update["message"].contains("text")) {
                continue;
            }
            
            const auto& msg = update["message"];
            long chatId = msg["chat"]["id"].get<long>();
            std::string text = msg["text"].get<std::string>();
            
            std::string username = "";
            if (msg.contains("from") && msg["from"].contains("username")) {
                username = msg["from"]["username"].get<std::string>();
            }
            
            messages.emplace_back(chatId, text, username);
            
            std::cout << "📨 Сообщение от " << chatId 
                      << "(@" << username << "): " << text << " (update_id: " << updateId << ")\n";
        }
        
        // ВАЖНО: Обновляем lastUpdateId для ВСЕХ обработанных обновлений
        // Это гарантирует, что они не вернутся при следующем запросе
        if (maxUpdateId > lastUpdateId) {
            lastUpdateId = maxUpdateId;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка парсинга: " << e.what() << std::endl;
    }
    
    return messages;
}

void TelegramHandler::sendMessage(long chatId, const std::string& text) {
    std::cout << "⚡ Отправляю ответ пользователю " << chatId << "...\n";
    std::cout << "DEBUG: Длина сообщения: " << text.length() << " символов\n";
    std::cout << "DEBUG: Первые 100 символов: " << text.substr(0, std::min(100UL, text.length())) << "\n";
    
    // Используем curl с --data-urlencode для правильного URL-кодирования
    std::string url = "https://api.telegram.org/bot" + botToken + "/sendMessage";
    std::string cmd = "curl -s -X POST \"" + url + "\"";
    cmd += " -d \"chat_id=" + std::to_string(chatId) + "\"";
    cmd += " --data-urlencode \"text=" + text + "\"";
    
    execCommand(cmd);
    
    std::cout << "✅ Отправлено " << chatId << ": " 
              << (text.length() > 50 ? text.substr(0, 50) + "..." : text) 
              << std::endl;
}

std::string TelegramHandler::getBotInfo() const {
    std::string response = sendRequest("getMe");
    
    if (response.empty()) {
        return "Информация о боте недоступна";
    }
    
    try {
        json j = json::parse(response);
        if (j["ok"].get<bool>()) {
            const auto& botInfo = j["result"];
            std::stringstream ss;
            ss << "🤖 Бот: " << botInfo["first_name"].get<std::string>() << "\n";
            ss << "Username: @" << botInfo["username"].get<std::string>() << "\n";
            ss << "ID: " << botInfo["id"].get<long>();
            return ss.str();
        }
    } catch (...) {
        // ignore
    }
    
    return "Информация о боте недоступна";
}

bool TelegramHandler::testConnection() const {
    std::string response = sendRequest("getMe");
    
    if (response.empty()) {
        return false;
    }
    
    try {
        json j = json::parse(response);
        return j["ok"].get<bool>();
    } catch (...) {
        return false;
    }
}

std::string TelegramHandler::getHelpText() {
    return "📋 КриптоБот - Помощь\n\n"
           "Основные команды:\n"
           "/start - Начать работу\n"
           "/help - Эта справка\n"
           "/list - Список криптовалют\n"
           "/price [СИМВОЛ] - Цена валюты\n"
           "/convert [СУММА] [ИЗ] to [В] - Конвертация\n"
           "/alert [СИМВОЛ] [above/below] [ЦЕНА] - Оповещение\n"
           "/myalerts - Мои оповещения\n"
           "/mystats - Моя статистика\n"
           "/stats - Статистика бота\n\n"
           "Примеры:\n"
           "/price BTC\n"
           "/convert 1 BTC to USD\n"
           "/alert ETH below 3000";
}

std::string TelegramHandler::getWelcomeText(const std::string& name) {
    return "👋 Привет, " + name + "!\n Добро пожаловать в КриптоБот \n Используйте /help для списка команд\n";
}