#include "CryptoBot.hpp"
#include <iostream>
#include <csignal> 


int main() {
    signal(SIGINT, CryptoBot::handleSignal);
    signal(SIGTERM, CryptoBot::handleSignal);

    std::string botToken = CryptoBot::readBotToken();
    if (botToken.empty()) {
        std::cerr << "❌ Токен бота не найден" << std::endl;
        return 1;
    }

    CryptoBot bot(botToken);
    bot.run();

    std::cout << "Завершение работы бота..." << std::endl;
    return 0;
}