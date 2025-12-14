#include "TelegramHandler.hpp"
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <thread>
#include <algorithm>  


std::string TelegramHandler::execCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return result;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::vector<Message> TelegramHandler::getMessages() {
    std::vector<Message> messages;
    std::string url = "https://api.telegram.org/bot" + botToken + "/getUpdates";
    std::string offset = "?offset=" + std::to_string(lastUpdateId + 1);
    std::string limit = "&limit=100";
    std::string timeout = "&timeout=30";
    std::string cmd = "curl -s -X GET \"" + url + offset + limit + timeout + "\"";

    std::string response = execCommand(cmd);

    try {
        json j = json::parse(response);

        if (!j.contains("ok") || !j["ok"]) {
            return messages;
        }

        if (!j.contains("result")) return messages;

        for (const auto& item : j["result"]) {
            if (item.contains("message")) {
                const auto& msg = item["message"];
                long chatId = msg["chat"].value("id", 0L);
                std::string text = msg.value("text", "");
                std::string username = msg["from"].value("username", "");
                long messageId = msg.value("message_id", 0L);
                long updateId = item.value("update_id", 0L);

                messages.push_back({chatId, text, username, messageId});
                lastUpdateId = std::max(lastUpdateId, updateId);
            }
        }
    }
    catch (const std::exception&) {
        // Молча игнорируем ошибку парсинга (можно добавить лог в файл, если нужно)
    }

    return messages;
}

void TelegramHandler::sendMessage(long chatId, const std::string& text) {
    std::string url = "https://api.telegram.org/bot" + botToken + "/sendMessage";
    std::string cmd = "curl -s -X POST \"" + url + "\"";
    cmd += " -d \"chat_id=" + std::to_string(chatId) + "\"";
    cmd += " --data-urlencode \"text=" + text + "\"";
    execCommand(cmd);
}

std::string TelegramHandler::getWelcomeText(const std::string& name) {
    std::stringstream ss;
    ss << "Привет, " << name << "! 👋\n";
    ss << "Я — CryptoBot, ваш помощник в мире криптовалют.\n\n";
    ss << "Используй /help, чтобы посмотреть список команд.";
    return ss.str();
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