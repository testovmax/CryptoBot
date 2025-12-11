#include "CurrencyManager.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <algorithm>
#include <random>
#include <ctime>
#include <ios>

CurrencyManager::CurrencyManager() {
    initializeCurrencies();
    updatePrices();
}

void CurrencyManager::initializeCurrencies() {
    std::lock_guard<std::mutex> lock(mutex);
    
    // Используй ту же карту что и в generateMockPrice
    std::map<std::string, std::pair<std::string, double>> cryptoMap = {
        {"BTC", {"Bitcoin", 93353.0}},
        {"ETH", {"Ethereum", 3395.35}},
        {"BNB", {"Binance Coin", 904.38}},
        {"SOL", {"Solana", 141.13}},
        {"LTC", {"Litecoin", 86.39}},
        {"LINK", {"Chainlink", 14.7}}
    };
    
    for (const auto& [symbol, data] : cryptoMap) {
        const auto& [name, price] = data;
        currencies[symbol] = {symbol, name, price, 0.0, "Never"};
    }
}

double CurrencyManager::generateMockPrice(const std::string& symbol) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::map<std::string, double> basePrices = {
        {"BTC", 93353.0}, {"ETH", 3395.35}, {"BNB", 904.38}, 
        {"SOL", 141.13},
         {"LTC", 86.39},
        {"LINK", 14.7}
    };
    
    if (basePrices.find(symbol) != basePrices.end()) {
        std::uniform_real_distribution<> dis(-0.02, 0.03);
        return basePrices[symbol] * (1.0 + dis(gen));
    }
    
    return 100.0;
}

Currency CurrencyManager::getCurrency(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mutex);
    std::string sym = symbol;
    std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
    
    auto it = currencies.find(sym);
    if (it != currencies.end()) {
        return it->second;
    }
    
    return {"", "Unknown", 0.0, 0.0, ""};
}

std::vector<Currency> CurrencyManager::getAllCurrencies() const {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<Currency> result;
    
    for (const auto& [symbol, currency] : currencies) {
        result.push_back(currency);
    }
    
    return result;
}

bool CurrencyManager::currencyExists(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mutex);
    std::string sym = symbol;
    std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
    return currencies.find(sym) != currencies.end();
}

void CurrencyManager::updatePrices() {
    std::lock_guard<std::mutex> lock(mutex);
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // Обновляем только существующие валюты, не добавляем новые
    for (auto& [symbol, currency] : currencies) {
        double newPrice = generateMockPrice(symbol);
        // Проверяем, что цена разумная (не слишком маленькая)
        if (newPrice > 0.01) {
            currency.price = newPrice;
        }
        
        std::uniform_real_distribution<> dis(-10.0, 15.0);
        currency.change24h = dis(gen);
        
        time_t now = time(nullptr);
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", std::localtime(&now));
        currency.lastUpdated = timeStr;
    }
}

double CurrencyManager::convert(double amount, const std::string& from, 
                               const std::string& to) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::string fromUpper = from;
    std::string toUpper = to;
    std::transform(fromUpper.begin(), fromUpper.end(), fromUpper.begin(), ::toupper);
    std::transform(toUpper.begin(), toUpper.end(), toUpper.begin(), ::toupper);
    
    auto fromIt = currencies.find(fromUpper);
    auto toIt = currencies.find(toUpper);
    
    if (fromIt == currencies.end() || toIt == currencies.end()) {
        return -1.0;
    }
    
    return (amount * fromIt->second.price) / toIt->second.price;
}

std::string CurrencyManager::formatCurrencyList() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    // Защита от использования старого кода - проверяем количество валют
    if (currencies.size() != 6) {
        std::cerr << "ERROR: Неправильное количество валют: " << currencies.size() << " (ожидается 6)\n";
        std::cerr << "Валюты: ";
        for (const auto& [symbol, currency] : currencies) {
            std::cerr << symbol << " ";
        }
        std::cerr << "\n";
    }
    
    std::stringstream ss;
    ss << "📊 Доступные криптовалюты:\n\n";
    
    int index = 1;
    for (const auto& [symbol, currency] : currencies) {
        ss << index << ". " << currency.name << " (" << symbol << ")\n";
        ss << "   Цена: USD ";
        
        // Форматируем цену с правильной точностью
        // Используем отдельный stringstream для каждого значения
        std::stringstream priceStream;
        priceStream << std::fixed;
        if (currency.price < 1.0) {
            priceStream << std::setprecision(4) << currency.price;
        } else {
            priceStream << std::setprecision(2) << currency.price;
        }
        std::string priceStr = priceStream.str();
        // Убираем ведущие нули перед точкой (например, "07.73" -> "7.73", "00.40" -> "0.40")
        while (priceStr.length() > 2 && priceStr[0] == '0' && priceStr[1] != '.') {
            priceStr = priceStr.substr(1);
        }
        // Исправляем ".11" -> "0.11"
        if (priceStr.length() > 0 && priceStr[0] == '.') {
            priceStr = "0" + priceStr;
        }
        ss << priceStr;
        
        // Форматируем изменение за 24ч
        std::stringstream changeStream;
        changeStream << std::fixed << std::setprecision(2) << currency.change24h;
        if (currency.change24h >= 0) {
            ss << " 📈 +" << changeStream.str() << "%";
        } else {
            ss << " 📉 " << changeStream.str() << "%";
        }
        
        ss << "\n\n";
        index++;
    }
    
    return ss.str();
}

std::string CurrencyManager::formatCurrencyInfo(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::string sym = symbol;
    std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
    
    auto it = currencies.find(sym);
    if (it == currencies.end()) {
        return "❌ Валюта не найдена: " + symbol;
    }
    
    const Currency& currency = it->second;
    
    std::stringstream ss;
    ss << "💰 " << currency.name << " (" << currency.symbol << ")\n\n";
    ss << "💵 Цена: USD ";
    if (currency.price < 1.0) {
        ss << std::fixed << std::setprecision(4) << currency.price;
    } else {
        ss << std::fixed << std::setprecision(2) << currency.price;
    }
    ss << "\n";
    ss << "📈 Изменение за 24ч: ";
    
    if (currency.change24h >= 0) {
        ss << "🟢 +" << std::fixed << std::setprecision(2) << currency.change24h << "%\n";
    } else {
        ss << "🔴 " << std::fixed << std::setprecision(2) << currency.change24h << "%\n";
    }
    
    ss << "\n🌍 В других валютах:\n";
    ss << "• " << std::fixed << std::setprecision(0) << (currency.price * 75) << " RUB\n";
    ss << "• " << std::fixed << std::setprecision(0) << (currency.price * 0.85) << " EUR\n";
    ss << "• " << std::fixed << std::setprecision(0) << (currency.price * 420) << " KZT\n";
    ss << "• " << std::fixed << std::setprecision(0) << (currency.price * 27.8) << " UAH\n";
    
    ss << "\n⏰ Обновлено: " << currency.lastUpdated;
    
    return ss.str();
}

std::string CurrencyManager::formatConversion(double amount, const std::string& from, 
                                             const std::string& to) const {
    double result = convert(amount, from, to);
    
    if (result < 0) {
        return "❌ Ошибка конвертации";
    }
    
    std::stringstream ss;
    ss << "🔄 Конвертация:\n\n";
    ss << std::fixed << std::setprecision(6) << amount << " " << from << " =\n";
    ss << std::fixed << std::setprecision(6) << result << " " << to << "\n\n";
    
    Currency fromCurrency = getCurrency(from);
    if (fromCurrency.name != "Unknown") {
        ss << "Курс: 1 " << from << " = " 
           << std::fixed << std::setprecision(6) << (result / amount) << " " << to;
    }
    
    return ss.str();
}

int CurrencyManager::getCurrencyCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return currencies.size();
}