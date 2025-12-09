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
target_compile_options(crypto_bot PRIVATE -Wall -Wextra)

# Подключаем JSON библиотеку (заголовочный файл)
target_include_directories(crypto_bot PRIVATE src)

message("Сборка КриптоБота")