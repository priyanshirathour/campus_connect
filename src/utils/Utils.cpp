#include "utils/Utils.h"

#include <atomic>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iostream>
#include <limits>
#ifdef _WIN32
#include <conio.h>
#endif

std::string nowTimestamp() {
    // Millisecond precision preserves the normal FIFO ordering when several
    // registrations are created during the same second.
    using namespace std::chrono;
    const auto current = system_clock::now();
    const auto millis = duration_cast<milliseconds>(current.time_since_epoch()) % 1000;
    const std::time_t now = system_clock::to_time_t(current);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
    std::string result(buffer);
    result += ".";
    const auto millisecondsPart = millis.count();
    if (millisecondsPart < 100) result += "0";
    if (millisecondsPart < 10) result += "0";
    result += std::to_string(millisecondsPart);
    return result;
}

std::string makeId(const std::string& prefix) {
    static std::atomic<unsigned long long> sequence{0};
    const auto ticks = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return prefix + "-" + std::to_string(ticks) + "-" + std::to_string(++sequence);
}

std::string toLowerCopy(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string inputLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

std::string inputSecret(const std::string& prompt) {
    std::cout << prompt;
    std::string secret;
#ifdef _WIN32
    while (true) {
        const int character = _getch();
        if (character == '\r') {
            break;
        }
        if (character == '\b') {
            if (!secret.empty()) {
                secret.pop_back();
            }
            continue;
        }
        if (character == 0 || character == 224) {
            _getch();
            continue;
        }
        if (character >= 32 && character <= 126) {
            secret.push_back(static_cast<char>(character));
        }
    }
    std::cout << '\n';
#else
    std::getline(std::cin, secret);
#endif
    return secret;
}

int inputInt(const std::string& prompt, int minimum, int maximum) {
    while (true) {
        std::cout << prompt;
        int value = 0;
        if (std::cin >> value && value >= minimum && value <= maximum) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        std::cout << "Please enter a number from " << minimum << " to " << maximum << ".\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}
