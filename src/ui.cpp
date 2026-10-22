#include "ui.h"
#include "config.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

void UI::clearScreen() {
#if FLOWDESK_WINDOWS
    std::system(FLOWDESK_CLEAR);
#else
    /* ANSI reset keeps the UI tidy without a shell call */
    std::cout << "\033[2J\033[H";
#endif
}

void UI::rule() {
    std::cout << C_LINE << "  " << std::string(FLOWDESK_UI_WIDTH, '=') << '\n' << C_RESET;
}

void UI::thinRule() {
    std::cout << C_LINE << "  " << std::string(FLOWDESK_UI_WIDTH, '-') << '\n' << C_RESET;
}

std::string UI::center(const std::string& text, int width) {
    if (static_cast<int>(text.size()) >= width) {
        return text.substr(0, static_cast<std::size_t>(width));
    }
    const int left = (width - static_cast<int>(text.size())) / 2;
    const int right = width - static_cast<int>(text.size()) - left;
    return std::string(static_cast<std::size_t>(left), ' ') + text +
           std::string(static_cast<std::size_t>(right), ' ');
}

void UI::banner() {
    const int inner = FLOWDESK_UI_WIDTH - 2;
    const std::string edge = "+" + std::string(static_cast<std::size_t>(inner), '=') + "+";
    const std::string title = std::string(FLOWDESK_TITLE) + "  |  v" + FLOWDESK_VERSION;

    std::cout << '\n'
              << C_LINE << "  " << edge << '\n'
              << "  |" << C_RESET << C_TITLE << center(title, inner) << C_RESET
              << C_LINE << "|\n"
              << "  |" << C_RESET << C_MUTED << center(FLOWDESK_SUBTITLE, inner) << C_RESET
              << C_LINE << "|\n"
              << "  " << edge << '\n' << C_RESET;
}

void UI::section(const std::string& title, const std::string& hint) {
    std::cout << '\n' << C_TITLE << "  >> " << title << C_RESET;
    if (!hint.empty()) {
        std::cout << C_MUTED << "   (" << hint << ")" << C_RESET;
    }
    std::cout << '\n';
    thinRule();
}

void UI::pause() {
    std::cout << C_MUTED << "\n  [ Press Enter to return to the menu ] " << C_RESET;
    std::string wait;
    if (!std::getline(std::cin, wait)) {
        std::cin.clear();
    }
}

void UI::notice(const std::string& text) {
    std::cout << C_MUTED << "  " << text << C_RESET << '\n';
}

void UI::warn(const std::string& text) {
    std::cout << C_WARN << "  [!] " << text << C_RESET << '\n';
}

void UI::ok(const std::string& text) {
    std::cout << C_GOOD << "  [OK] " << text << C_RESET << '\n';
}

std::string UI::readLine(const std::string& prompt) {
    std::cout << C_ACCENT << "  > " << prompt << C_RESET;
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cin.clear();
        return {};
    }
    return line;
}

std::string UI::readLineOrDefault(const std::string& prompt, const std::string& fallback) {
    const std::string line = readLine(prompt);
    return line.empty() ? fallback : line;
}

int UI::readInt(const std::string& prompt, int min, int max) {
    while (std::cin) {
        const std::string line = readLine(prompt);
        try {
            const int value = std::stoi(line);
            if (value >= min && value <= max) {
                return value;
            }
        } catch (...) {
        }
        if (!std::cin) {
            break;
        }
        warn("Please enter a number between " + std::to_string(min) +
             " and " + std::to_string(max) + ".");
    }
    return min;
}

double UI::readDouble(const std::string& prompt, double min, double max) {
    while (std::cin) {
        const std::string line = readLine(prompt);
        try {
            const double value = std::stod(line);
            if (value >= min && value <= max) {
                return value;
            }
        } catch (...) {
        }
        if (!std::cin) {
            break;
        }
        warn("Please enter a valid amount.");
    }
    return min;
}

bool UI::confirm(const std::string& prompt) {
    const std::string line = readLine(prompt + " [y/N]: ");
    return !line.empty() && (line[0] == 'y' || line[0] == 'Y');
}

std::string UI::money(double value) {
    /* "Rs 12,345.67" with thousands separators; negatives as "-Rs 49.00" */
    std::ostringstream fixed;
    fixed << std::fixed << std::setprecision(2) << (value < 0 ? -value : value);
    const std::string plain = fixed.str();
    const std::size_t dot = plain.find('.');
    std::string whole = plain.substr(0, dot);
    const std::string cents = plain.substr(dot);

    std::string grouped;
    int count = 0;
    for (auto it = whole.rbegin(); it != whole.rend(); ++it) {
        if (count > 0 && count % 3 == 0) {
            grouped.insert(grouped.begin(), ',');
        }
        grouped.insert(grouped.begin(), *it);
        ++count;
    }
    return std::string(value < 0 ? "-" : "") + FLOWDESK_CURRENCY + grouped + cents;
}

std::string UI::today() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#if FLOWDESK_WINDOWS
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d");
    return out.str();
}
