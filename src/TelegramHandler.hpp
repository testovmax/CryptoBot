// src/TelegramHandler.hpp
#pragma once

#include <string>
#include <vector>

#include "json.hpp"
using json = nlohmann::json;

struct Message {
    long chatId;
    std::string text;
    std::string username;
    long messageId;
};

class TelegramHandler {
private:
    std::string botToken;
    long lastUpdateId;

    std::string execCommand(const std::string& cmd);
    std::string extractJsonField(const std::string& json, const std::string& field);

public:
    TelegramHandler(const std::string& token);
    std::vector<Message> getMessages();
    void sendMessage(long chatId, const std::string& text);

    static std::string getWelcomeText(const std::string& name);
    static std::string getHelpText();
};