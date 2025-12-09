#ifndef USER_MANAGER_HPP
#define USER_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <mutex>

struct User {
    long id;
    std::string username;
    time_t lastActivity;
    int commandsUsed;
    std::vector<std::string> favorites;
    
    User(long uid = 0, const std::string& uname = "");
    std::string getInfo() const;
};

class UserManager {
private:
    std::map<long, User> users;
    mutable std::mutex mutex;
    
public:
    UserManager();
    
    // Управление пользователями
    User* getUser(long userId);
    User* createUser(long userId, const std::string& username);
    bool userExists(long userId) const;
    void updateUserActivity(long userId);
    
    // Статистика
    void recordCommand(long userId);
    int getTotalUsers() const;
    int getTotalCommands() const;
    std::string getStats() const;
    std::string getUserStats(long userId) const;
    
    // Избранное
    void addFavorite(long userId, const std::string& crypto);
    void removeFavorite(long userId, const std::string& crypto);
    std::vector<std::string> getFavorites(long userId) const;
};

#endif