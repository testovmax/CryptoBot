#include "UserManager.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

User::User(long uid, const std::string& uname) 
    : id(uid), username(uname), lastActivity(time(nullptr)), commandsUsed(0) {}

std::string User::getInfo() const {
    std::stringstream ss;
    ss << "👤 Пользователь ID: " << id << "\n";
    if (!username.empty()) ss << "Username: @" << username << "\n";
    ss << "Команд использовано: " << commandsUsed << "\n";
    
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%d.%m.%Y %H:%M", 
                  std::localtime(&lastActivity));
    ss << "Последняя активность: " << timeStr << "\n";
    
    if (!favorites.empty()) {
        ss << "Избранное: ";
        for (size_t i = 0; i < favorites.size(); i++) {
            ss << favorites[i];
            if (i < favorites.size() - 1) ss << ", ";
        }
    }
    
    return ss.str();
}

UserManager::UserManager() {}

User* UserManager::getUser(long userId) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = users.find(userId);
    if (it != users.end()) {
        it->second.lastActivity = time(nullptr);
        return &(it->second);
    }
    
    return nullptr;
}

User* UserManager::createUser(long userId, const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex);
    
    users[userId] = User(userId, username);
    return &users[userId];
}

bool UserManager::userExists(long userId) const {
    std::lock_guard<std::mutex> lock(mutex);
    return users.find(userId) != users.end();
}

void UserManager::updateUserActivity(long userId) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = users.find(userId);
    if (it != users.end()) {
        it->second.lastActivity = time(nullptr);
    }
}

void UserManager::recordCommand(long userId) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = users.find(userId);
    if (it != users.end()) {
        it->second.commandsUsed++;
        it->second.lastActivity = time(nullptr);
    }
}

int UserManager::getTotalUsers() const {
    std::lock_guard<std::mutex> lock(mutex);
    return users.size();
}

int UserManager::getTotalCommands() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    int total = 0;
    for (const auto& [id, user] : users) {
        total += user.commandsUsed;
    }
    
    return total;
}

std::string UserManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::stringstream ss;
    ss << "📊 Статистика пользователей:\n\n";
    ss << "Всего пользователей: " << users.size() << "\n";
    
    int totalCommands = 0;
    int activeLastHour = 0;
    time_t oneHourAgo = time(nullptr) - 3600;
    
    for (const auto& [id, user] : users) {
        totalCommands += user.commandsUsed;
        if (user.lastActivity >= oneHourAgo) {
            activeLastHour++;
        }
    }
    
    ss << "Активных (за час): " << activeLastHour << "\n";
    ss << "Всего команд: " << totalCommands << "\n";
    
    if (!users.empty()) {
        double avgCommands = static_cast<double>(totalCommands) / users.size();
        ss << "Среднее команд на пользователя: " 
           << std::fixed << std::setprecision(1) << avgCommands << "\n";
    }
    
    return ss.str();
}

std::string UserManager::getUserStats(long userId) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = users.find(userId);
    if (it == users.end()) {
        return "❌ Пользователь не найден";
    }
    
    return it->second.getInfo();
}

void UserManager::addFavorite(long userId, const std::string& crypto) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = users.find(userId);
    if (it != users.end()) {
        std::string upperCrypto = crypto;
        std::transform(upperCrypto.begin(), upperCrypto.end(), 
                      upperCrypto.begin(), ::toupper);
        
        if (std::find(it->second.favorites.begin(), 
                     it->second.favorites.end(), upperCrypto) == it->second.favorites.end()) {
            it->second.favorites.push_back(upperCrypto);
        }
    }
}

void UserManager::removeFavorite(long userId, const std::string& crypto) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = users.find(userId);
    if (it != users.end()) {
        std::string upperCrypto = crypto;
        std::transform(upperCrypto.begin(), upperCrypto.end(), 
                      upperCrypto.begin(), ::toupper);
        
        auto favIt = std::find(it->second.favorites.begin(), 
                              it->second.favorites.end(), upperCrypto);
        if (favIt != it->second.favorites.end()) {
            it->second.favorites.erase(favIt);
        }
    }
}

std::vector<std::string> UserManager::getFavorites(long userId) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = users.find(userId);
    if (it != users.end()) {
        return it->second.favorites;
    }
    
    return {};
}