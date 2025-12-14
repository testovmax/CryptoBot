#include "UserManager.hpp"
#include <ctime>

UserManager::UserData::UserData(const std::string& name)
    : username(name), commandCount(0), favorites(), firstSeen(time(nullptr)) {}

bool UserManager::userExists(long userId) const {
    std::lock_guard<std::mutex> lock(mutex);
    return users.find(userId) != users.end();
}

void UserManager::createUser(long userId, const std::string& username) {
    users.try_emplace(userId, username);
}

void UserManager::recordCommand(long userId) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = users.find(userId);
    if (it != users.end()) {
        it->second.commandCount++;
    }
}

void UserManager::addFavorite(long userId, const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = users.find(userId);
    if (it != users.end()) {
        it->second.favorites.insert(symbol);
    }
}

std::string UserManager::getUserStats(long userId) const {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = users.find(userId);
    if (it == users.end()) {
        return "❌ Пользователь не найден";
    }
    const auto& data = it->second;
    std::stringstream ss;
    ss << "📊 Ваша статистика:\n\n";
    ss << "Имя: " << data.username << "\n";
    ss << "Команд использовано: " << data.commandCount << "\n";
    ss << "Любимые валюты: ";
    if (data.favorites.empty()) {
        ss << "нет";
    } else {
        bool first = true;
        for (const auto& fav : data.favorites) {
            if (!first) ss << ", ";
            ss << fav;
            first = false;
        }
    }
    return ss.str();
}

std::string UserManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex);
    std::stringstream ss;
    ss << "🌐 Статистика бота:\n";
    ss << "Пользователей: " << users.size();
    return ss.str();
}