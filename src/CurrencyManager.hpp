#ifndef CURRENCY_MANAGER_HPP
#define CURRENCY_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <mutex>

struct Currency {
    std::string symbol;
    std::string name;
    double price;
    double change24h;
    std::string lastUpdated;
};

class CurrencyManager {
private:
    std::map<std::string, Currency> currencies;
    mutable std::mutex mutex;
    
    void initializeCurrencies();
    double generateMockPrice(const std::string& symbol);
    
public:
    CurrencyManager();
    
    // Получение данных
    Currency getCurrency(const std::string& symbol)const;
    std::vector<Currency> getAllCurrencies() const;
    bool currencyExists(const std::string& symbol) const;
    
    // Обновление цен
    void updatePrices();
    
    // Конвертация
    double convert(double amount, const std::string& from, const std::string& to) const;
    
    // Форматирование
    std::string formatCurrencyList() const;
    std::string formatCurrencyInfo(const std::string& symbol);
    std::string formatConversion(double amount, const std::string& from, 
                                const std::string& to) const;
    
    // Статистика
    int getCurrencyCount() const;
};

#endif