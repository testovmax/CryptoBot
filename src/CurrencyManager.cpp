// src/CurrencyManager.cpp
#include "CurrencyManager.hpp"
#include <sstream>
#include <iomanip>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include "json.hpp"  
#include <iostream>

using json = nlohmann::json;

// ID валют на CoinGecko
static const std::map<std::string, std::string> COINGECKO_IDS = {
    {"BTC", "bitcoin"},
    {"ETH", "ethereum"},
    {"BNB", "binancecoin"},
    {"SOL", "solana"},
    {"LTC", "litecoin"},
    {"LINK", "chainlink"}
};

// Вспомогательная функция для выполнения команды
static std::string execCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return result;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

Currency::Currency(const std::string& s, const std::string& n, double p, double c, const std::string& l)
    : symbol(s), name(n), price(p), change24h(c), lastUpdated(l) {}

CurrencyManager::CurrencyManager() {
    initializeCurrencies();
}

void CurrencyManager::initializeCurrencies() {
    std::lock_guard<std::mutex> lock(mutex);
    for (const auto& [symbol, name] : {
        std::make_pair("BTC", "Bitcoin"),
        std::make_pair("ETH", "Ethereum"),
        std::make_pair("BNB", "Binance Coin"),
        std::make_pair("SOL", "Solana"),
        std::make_pair("LTC", "Litecoin"),
        std::make_pair("LINK", "Chainlink")
    }) {
        currencies[symbol] = {symbol, name, 0.0, 0.0, "—"};
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

void CurrencyManager::updatePrices() {
    std::lock_guard<std::mutex> lock(mutex);

    std::string ids;
    for (const auto& [symbol, id] : COINGECKO_IDS) {
        if (!ids.empty()) ids += ",";
        ids += id;
    }

    std::string url = "https://api.coingecko.com/api/v3/simple/price?ids=" + ids +
                      "&vs_currencies=usd&include_24hr_change=true";

    std::string cmd = "curl -s -m 10 \"" + url + "\"";
    std::string response = execCommand(cmd);

    std::cout << "\n🔍 [updatePrices] URL: " << url << "\n";
    std::cout << "📥 [updatePrices] Response: " << (response.empty() ? "(пусто)" : response) << "\n";

    try {
        json j = json::parse(response);

        std::cout << "✅ [updatePrices] JSON успешно распаршен\n";

        for (auto& [symbol, currency] : currencies) {
            auto it = COINGECKO_IDS.find(symbol);
            if (it == COINGECKO_IDS.end()) {
                std::cout << "🟡 [updatePrices] Нет CoinGecko ID для: " << symbol << "\n";
                continue;
            }

            std::string id = it->second;
            std::cout << "📡 [updatePrices] Ищем: " << id << "\n";

            if (j.contains(id)) {
                auto& data = j[id];
                std::cout << "📥 [updatePrices] Найдено: " << id << " → " << data.dump() << "\n";

                if (data.contains("usd")) {
                    double price = data["usd"].get<double>();
                    currency.price = price;
                    std::cout << "📊 [updatePrices] Установлена цена " << symbol << " = " << price << "\n";
                } else {
                    std::cout << "❌ [updatePrices] Нет поля 'usd' в " << id << "\n";
                }

                if (data.contains("usd_24h_change")) {
                    currency.change24h = data["usd_24h_change"].get<double>();
                }
            } else {
                std::cout << "❌ [updatePrices] Не найдено в ответе: " << id << "\n";
            }

            time_t now = time(nullptr);
            char timeStr[10];
            std::strftime(timeStr, sizeof(timeStr), "%H:%M", std::localtime(&now));
            currency.lastUpdated = timeStr;
        }
    }
    catch (const std::exception& e) {
        std::cout << "❌ [updatePrices] JSON Parse Error: " << e.what() << "\n";
        std::cout << "Raw: " << response << "\n";
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
            {"KZT", 420.0}
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