КРИПТО БОТ - КУРСОВАЯ РАБОТА
===========================

Структура проекта:
- main.cpp              - Главный файл программы
- CryptoBot.hpp/cpp     - Основной класс бота
- CurrencyManager.hpp/cpp - Управление криптовалютами
- UserManager.hpp/cpp   - Управление пользователями
- TelegramHandler.hpp/cpp - Работа с Telegram API

Сборка:
1. mkdir build
2. cd build
3. cmake ..
4. make

Запуск:
./crypto_bot

Для первого запуска потребуется токен Telegram бота.

Основные команды бота:
/start    - Начать работу
/help     - Помощь
/list     - Список криптовалют
/price BTC - Цена Bitcoin
/convert 1 BTC to USD - Конвертация
/alert BTC above 50000 - Оповещение о цене
/myalerts - Мои оповещения
/mystats  - Моя статистика
/stats    - Статистика бота

Команды управления в консоли:
status - Статус бота
update - Обновить цены
alerts - Проверить оповещения
exit   - Выйти