#pragma once

#include <string>
#include <map>
#include <mutex>
#include <random>

struct Currency {
    std::string symbol;
    std::string name;
    double price;
    double change24h;
    std::string lastUpdated;

    Currency() = default;
    Currency(const std::string& s, const std::string& n, double p, double c, const std::string& l);
};

class CurrencyManager {
private:
    std::map<std::string, Currency> currencies;
    mutable std::mutex mutex;

    std::string getCurrencyName(const std::string& symbol) const;
    double generateMockPrice(const std::string& symbol);
    static std::mt19937 gen;

public:
    CurrencyManager();
    void initializeCurrencies();
    void updatePrices();
    bool currencyExists(const std::string& symbol) const;
    Currency getCurrency(const std::string& symbol) const;
    int getCurrencyCount() const;

    std::string formatCurrencyList() const;
    std::string formatCurrencyInfo(const std::string& symbol) const;
    std::string formatConversion(double amount, const std::string& from, const std::string& to) const;
};