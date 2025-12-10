#include "CryptoBot.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <csignal>
#include <atomic>




std::string readBotToken() {
    // Пробуем прочитать из файла
    std::ifstream tokenFile("bot_token.txt");
    if (tokenFile.is_open()) {
        std::string token;
        std::getline(tokenFile, token);
        tokenFile.close();
        
        if (!token.empty()) {
            std::cout << "Токен прочитан из файла bot_token.txt\n";
            return token;
        }
    }
    
    // Запрашиваем у пользователя
    
    std::string token;
    std::cout << "Введите токен бота: ";
    std::getline(std::cin, token);
    
    // Сохраняем для будущих запусков
    std::ofstream outFile("bot_token.txt");
    if (outFile.is_open()) {
        outFile << token;
        outFile.close();
        std::cout << "Токен сохранен в bot_token.txt\n";
    }
    
    return token;
}

int main() {
    
    // Устанавливаем обработчик Ctrl+C
    
    std::string botToken = readBotToken();
    
    if (botToken.empty()) {
        std::cerr << "ОШИБКА: Токен бота не может быть пустым!\n";
        return 1;
    }
    
    try {
        std::cout << "\n⏳ Инициализация бота...\n";
        CryptoBot bot(botToken);
        
        std::cout << "\n@CryptoLabuba_bot работает\n";

        
        // Запускаем главный цикл бота
        bot.run();
        
    } catch (const std::exception& e) {
        std::cerr << "\nОШИБКА: " << e.what() << "\n";
        std::cerr << "Бот не может быть запущен.\n";
        return 1;
    }

    return 0;
}