#include "CurrencyManager.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <ctime>

CurrencyManager::CurrencyManager() {
    initializeCurrencies();
    updatePrices();
}

void CurrencyManager::initializeCurrencies() {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<std::pair<std::string, std::string>> cryptoList = {
        {"BTC", "Bitcoin"}, {"ETH", "Ethereum"}, {"BNB", "Binance Coin"},
        {"SOL", "Solana"}, {"ADA", "Cardano"}, {"XRP", "Ripple"},
        {"DOGE", "Dogecoin"}, {"DOT", "Polkadot"}, {"LTC", "Litecoin"},
        {"LINK", "Chainlink"}
    };
    
    for (const auto& [symbol, name] : cryptoList) {
        currencies[symbol] = {symbol, name, 0.0, 0.0, "Never"};
    }
}

double CurrencyManager::generateMockPrice(const std::string& symbol) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::map<std::string, double> basePrices = {
        {"BTC", 45000.0}, {"ETH", 2500.0}, {"BNB", 300.0},
        {"SOL", 100.0}, {"ADA", 0.5}, {"XRP", 0.5},
        {"DOGE", 0.08}, {"DOT", 7.0}, {"LTC", 70.0},
        {"LINK", 14.0}
    };
    
    if (basePrices.find(symbol) != basePrices.end()) {
        std::uniform_real_distribution<> dis(-0.02, 0.03);
        return basePrices[symbol] * (1.0 + dis(gen));
    }
    
    return 100.0;
}

Currency CurrencyManager::getCurrency(const std::string& symbol)const {
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
    
    for (auto& [symbol, currency] : currencies) {
        currency.price = generateMockPrice(symbol);
        
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
    
    std::stringstream ss;
    ss << "📊 Доступные криптовалюты:\n\n";
    
    int index = 1;
    for (const auto& [symbol, currency] : currencies) {
        ss << index << ". " << currency.name << " (" << symbol << ")\n";
        ss << "   Цена: $" << std::fixed << std::setprecision(2) << currency.price;
        
        if (currency.change24h >= 0) {
            ss << " 📈 +" << std::fixed << std::setprecision(2) << currency.change24h << "%";
        } else {
            ss << " 📉 " << std::fixed << std::setprecision(2) << currency.change24h << "%";
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
    ss << "Цена: $" << std::fixed << std::setprecision(2) << currency.price << "\n";
    ss << "Изменение 24ч: ";
    
    if (currency.change24h >= 0) {
        ss << "📈 +" << std::fixed << std::setprecision(2) << currency.change24h << "%\n";
    } else {
        ss << "📉 " << std::fixed << std::setprecision(2) << currency.change24h << "%\n";
    }
    
    ss << "\nПримерно:\n";
    ss << std::fixed << std::setprecision(0) << (currency.price * 75) << " RUB\n";
    ss << std::fixed << std::setprecision(0) << (currency.price * 0.85) << " EUR\n";
    ss << std::fixed << std::setprecision(0) << (currency.price * 420) << " KZT\n";
    
    ss << "\nОбновлено: " << currency.lastUpdated;
    
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