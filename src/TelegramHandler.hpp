#ifndef TELEGRAM_HANDLER_HPP
#define TELEGRAM_HANDLER_HPP

#include <string>
#include <vector>
#include <map>

struct TelegramMessage {
    long chatId;
    std::string text;
    std::string username;
    
    TelegramMessage(long id = 0, const std::string& txt = "", 
                   const std::string& user = "")
        : chatId(id), text(txt), username(user) {}
};

class TelegramHandler {
private:
    std::string botToken;
    long lastUpdateId;
    
    std::string execCommand(const std::string& cmd) const;
    std::string sendRequest(const std::string& method, const std::string& data = "") const;
    
public:
    TelegramHandler(const std::string& token);
    
    // Работа с сообщениями
    std::vector<TelegramMessage> getMessages();
    void sendMessage(long chatId, const std::string& text);
    
    // Информация
    std::string getBotInfo() const;
    bool testConnection() const;
    
    // Вспомогательные методы
    static std::string getHelpText();
    static std::string getWelcomeText(const std::string& name);
};

#endif