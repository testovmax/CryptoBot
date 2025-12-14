// src/CurrencyManager.cpp
#include "CurrencyManager.hpp"
#include <sstream>
#include <iomanip>
#include <random>
#include <ctime>
#include <algorithm>


std::mt19937 CurrencyManager::gen(std::random_device{}());

const inline std::map<std::string, double> BASE_PRICES = {
    {"BTC", 93353.0},
    {"ETH", 3395.35},
    {"BNB", 904.38},
    {"SOL", 141.13},
    {"LTC", 86.39},
    {"LINK", 14.7}
};

Currency::Currency(const std::string& s, const std::string& n, double p, double c, const std::string& l)
    : symbol(s), name(n), price(p), change24h(c), lastUpdated(l) {}

CurrencyManager::CurrencyManager() {
    initializeCurrencies();
}

void CurrencyManager::initializeCurrencies() {
    std::lock_guard<std::mutex> lock(mutex);
    for (const auto& [symbol, price] : BASE_PRICES) {
        time_t now = time(nullptr);
        char timeStr[10];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M", std::localtime(&now));
        currencies[symbol] = {symbol, getCurrencyName(symbol), price, 0.0, timeStr};
    }
}

std::string CurrencyManager::getCurrencyName(const std::string& symbol) const {
    static const std::map<std::string, std::string> names = {
        {"BTC", "Bitcoin"},
        {"ETH", "Ethereum"},
        {"BNB", "Binance Coin"},
        {"SOL", "Solana"},
        {"LTC", "Litecoin"},
        {"LINK", "Chainlink"}
    };
    auto it = names.find(symbol);
    return (it != names.end()) ? it->second : "Unknown";
}

double CurrencyManager::generateMockPrice(const std::string& symbol) {
    auto it = BASE_PRICES.find(symbol);
    if (it != BASE_PRICES.end()) {
        std::uniform_real_distribution<> dis(-0.02, 0.03);
        return it->second * (1.0 + dis(gen));  // ✅ gen доступен
    }
    return 100.0;
}

void CurrencyManager::updatePrices() {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& [symbol, currency] : currencies) {
        double newPrice = generateMockPrice(symbol);
        if (newPrice > 0.01) {
            currency.price = newPrice;
        }
        std::uniform_real_distribution<> dis(-10.0, 15.0);
        currency.change24h = dis(gen);

        time_t now = time(nullptr);
        char timeStr[10];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M", std::localtime(&now));
        currency.lastUpdated = timeStr;
    }
}

bool CurrencyManager::currencyExists(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mutex);
    return currencies.find(symbol) != currencies.end();
}

Currency CurrencyManager::getCurrency(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = currencies.find(symbol);
    return (it != currencies.end()) ? it->second : Currency{"", "Unknown", 0.0, 0.0, ""};
}

int CurrencyManager::getCurrencyCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return currencies.size();
}

std::string CurrencyManager::formatCurrencyList() const {
    std::lock_guard<std::mutex> lock(mutex);
    std::stringstream ss;
    ss << "📊 Доступные криптовалюты:\n\n";
    int i = 1;
    for (const auto& [symbol, currency] : currencies) {
        ss << i++ << ". " << currency.name << " (" << currency.symbol << ")\n";
        ss << "   Цена: USD ";
        if (currency.price < 1.0) {
            ss << std::fixed << std::setprecision(4) << currency.price;
        } else {
            ss << std::fixed << std::setprecision(2) << currency.price;
        }
        if (currency.change24h >= 0) {
            ss << " 📈 +" << std::fixed << std::setprecision(2) << currency.change24h << "%\n\n";
        } else {
            ss << " 📉 " << std::fixed << std::setprecision(2) << currency.change24h << "%\n\n";
        }
    }
    return ss.str();
}

std::string CurrencyManager::formatCurrencyInfo(const std::string& symbol) const {
    Currency currency = getCurrency(symbol);
    if (currency.name == "Unknown") {
        return "❌ Валюта не найдена: " + symbol;
    }

    std::stringstream ss;
    ss << "💰 " << currency.name << " (" << currency.symbol << ")\n\n";
    ss << "💵 Цена: USD ";
    if (currency.price < 1.0) {
        ss << std::fixed << std::setprecision(4) << currency.price;
    } else {
        ss << std::fixed << std::setprecision(2) << currency.price;
    }

    ss << "\n📈 Изменение за 24ч: ";
    if (currency.change24h >= 0) {
        ss << "🟢 +" << std::fixed << std::setprecision(2) << currency.change24h << "%\n";
    } else {
        ss << "🔴 " << std::fixed << std::setprecision(2) << currency.change24h << "%\n";
    }

    double rub = currency.price * 75.0;
    double eur = currency.price * 0.85;
    double kzt = currency.price * 420.0;

    ss << "\n🌍 В других валютах:\n";
    ss << "• " << std::fixed << std::setprecision(0) << rub << " RUB\n";
    ss << "• " << std::fixed << std::setprecision(0) << eur << " EUR\n";
    ss << "• " << std::fixed << std::setprecision(0) << kzt << " KZT\n";
    ss << "\n⏰ Обновлено: " << currency.lastUpdated;

    return ss.str();
}

std::string CurrencyManager::formatConversion(double amount, const std::string& from, const std::string& to) const {
    std::string fromSym = from, toSym = to;
    std::transform(fromSym.begin(), fromSym.end(), fromSym.begin(), ::toupper);
    std::transform(toSym.begin(), toSym.end(), toSym.begin(), ::toupper);

    const auto getFiatRate = [](const std::string& fiat) -> double {
        static const std::map<std::string, double> rates = {
            {"USD", 1.0}, {"RUB", 75.0}, {"EUR", 0.85},
            {"KZT", 420.0}, {"UAH", 27.8}
        };
        auto it = rates.find(fiat);
        return (it != rates.end()) ? it->second : -1.0;
    };

    double fromRate = getFiatRate(fromSym);
    double toRate = getFiatRate(toSym);

    if (currencyExists(fromSym) && toRate > 0) {
        Currency fromCurrency = getCurrency(fromSym);
        double result = amount * fromCurrency.price * toRate;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2)
           << amount << " " << fromSym << " = " << result << " " << to;
        return ss.str();
    }
    else if (fromRate > 0 && currencyExists(toSym)) {
        Currency toCurrency = getCurrency(toSym);
        double result = (amount / fromRate) / toCurrency.price;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(8)
           << amount << " " << fromSym << " = " << result << " " << to;
        return ss.str();
    }
    else if (currencyExists(fromSym) && currencyExists(toSym)) {
        Currency fromCurrency = getCurrency(fromSym);
        Currency toCurrency = getCurrency(toSym);
        double result = (amount * fromCurrency.price) / toCurrency.price;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(8)
           << amount << " " << fromSym << " = " << result << " " << to;
        return ss.str();
    }
    return "❌ Невозможно выполнить конвертацию";
}