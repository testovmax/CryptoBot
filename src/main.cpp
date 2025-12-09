#include "CryptoBot.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>


std::string readBotToken() {
    // Пробуем прочитать из файла
    std::ifstream tokenFile("bot_token.txt");
    if (tokenFile.is_open()) {
        std::string token;
        std::getline(tokenFile, token);
        tokenFile.close();
        
        if (!token.empty()) {
            std::cout << "✓ Токен прочитан из файла bot_token.txt\n";
            return token;
        }
    }
    
    // Запрашиваем у пользователя
    std::cout << "\n🤖 ТРЕБУЕТСЯ ТОКЕН TELEGRAM БОТА\n";
    std::cout << "================================\n";
    std::cout << "1. Откройте Telegram\n";
    std::cout << "2. Найдите @BotFather\n";
    std::cout << "3. Создайте бота командой /newbot\n";
    std::cout << "4. Придумайте имя боту\n";
    std::cout << "5. Скопируйте токен (выглядит так: 123456789:ABCdefGHIjklMNOpqrSTUvwxYZ)\n";
    std::cout << "================================\n\n";
    
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

void printConsoleCommands() {
    std::cout << "\nКОМАНДЫ УПРАВЛЕНИЯ В КОНСОЛИ:\n";
    std::cout << "===============================\n";
    std::cout << "status  - Показать статус бота\n";
    std::cout << "update  - Принудительно обновить цены\n";
    std::cout << "alerts  - Проверить оповещения\n";
    std::cout << "help    - Показать эту справку\n";
    std::cout << "exit    - Выйти из программы\n";
    std::cout << "===============================\n\n";
}


int main() {
    
    std::string botToken = readBotToken();
    
    if (botToken.empty()) {
        std::cerr << "ОШИБКА: Токен бота не может быть пустым!\n";
        return 1;
    }
    
    try {
        std::cout << "\nИнициализация бота...\n";
        CryptoBot bot(botToken);
        
        std::cout << "\nБОТ УСПЕШНО ИНИЦИАЛИЗИРОВАН!\n";
        std::cout << "================================\n\n";
        
        printConsoleCommands();
        
        std::cout << "\nБОТ ЗАПУЩЕН И РАБОТАЕТ\n";
        std::cout << "Для выхода введите 'exit' в консоли\n";
        std::cout << "================================\n\n";
        
        // Запускаем главный цикл бота
        bot.run();
        
    } catch (const std::exception& e) {
        std::cerr << "\nКРИТИЧЕСКАЯ ОШИБКА: " << e.what() << "\n";
        std::cerr << "Бот не может быть запущен.\n";
        return 1;
    }
    
    std::cout << "          РАБОТА БОТА ЗАВЕРШЕНА         \n";
    
    return 0;
}