#pragma once

#include <string>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <ctime>

class UserManager {
private:
    struct UserData {
        std::string username;
        int commandCount;
        std::set<std::string> favorites;
        time_t firstSeen;

        UserData() = default;
        UserData(const std::string& name);
    };

    std::map<long, UserData> users;
    mutable std::mutex mutex;

public:
    bool userExists(long userId) const;
    void createUser(long userId, const std::string& username);
    void recordCommand(long userId);
    void addFavorite(long userId, const std::string& symbol);
    std::string getUserStats(long userId) const;
    std::string getStats() const;
    std::map<long, UserData> getUsers() const;
};