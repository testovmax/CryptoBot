cmake_minimum_required(VERSION 3.10)
project(CryptoBotCourse)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исходные файлы
set(SOURCES
    src/main.cpp
    src/CryptoBot.cpp
    src/CurrencyManager.cpp
    src/UserManager.cpp
    src/TelegramHandler.cpp
)

# Создаем исполняемый файл
add_executable(crypto_bot ${SOURCES})

# Настройки компилятора
target_compile_options(crypto_bot PRIVATE -Wall -Wextra -pedantic)

# Пути для заголовков
target_include_directories(crypto_bot PRIVATE src)

# Информация о сборке
message("========================================")
message("КриптоБот - Telegram Only версия")
message("Управление: только через Telegram")
message("Выход: Ctrl+C в терминале")
message("========================================")