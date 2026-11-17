// utils.h - custom exceptions, templated input readers, date helpers, small formatting helpers
#pragma once
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace eco {

// ---------------------------------------------------------------- Custom exceptions
class TrackerError : public std::runtime_error {
public:
    explicit TrackerError(const std::string& m) : std::runtime_error(m) {}
};
class InputClosedError : public TrackerError {
public:
    InputClosedError() : TrackerError("Input stream closed") {}
};
class EntryNotFoundError : public TrackerError {
public:
    explicit EntryNotFoundError(int id) : TrackerError("No entry with ID " + std::to_string(id)) {}
};
class DataError : public TrackerError {
public:
    explicit DataError(const std::string& m) : TrackerError(m) {}
};

// ---------------------------------------------------------------- Templated input
// Reads a number of any arithmetic type within [lo, hi]; re-prompts until valid.
template <typename T>
T readValue(std::istream& in, std::ostream& out, const std::string& prompt, T lo, T hi) {
    static_assert(std::is_arithmetic<T>::value, "readValue needs an arithmetic type");
    T v;
    while (true) {
        out << prompt;
        if (in >> v && v >= lo && v <= hi) {
            in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return v;
        }
        if (in.eof()) throw InputClosedError();
        in.clear();
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        out << "  Enter a value between " << lo << " and " << hi << ".\n";
    }
}
template <typename T>
T readValue(const std::string& prompt, T lo, T hi) { return readValue<T>(std::cin, std::cout, prompt, lo, hi); }

inline std::string readLine(std::istream& in, std::ostream& out, const std::string& prompt) {
    std::string s;
    while (true) {
        out << prompt;
        if (!std::getline(in, s)) throw InputClosedError();
        if (!s.empty()) return s;
        out << "  Input cannot be empty.\n";
    }
}
inline std::string readLine(const std::string& prompt) { return readLine(std::cin, std::cout, prompt); }

// ---------------------------------------------------------------- Dates (day numbers since 1970-01-01)
inline long daysFromCivil(int y, int m, int d) {
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const long yoe = y - era * 400;
    const long doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}
inline void civilFromDays(long z, int& y, int& m, int& d) {
    z += 719468;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const long doe = z - era * 146097;
    const long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const long mp = (5 * doy + 2) / 153;
    d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    y = static_cast<int>(yoe + era * 400 + (m <= 2));
}
inline std::string fromDay(long day) {
    int y, m, d;
    civilFromDays(day, y, m, d);
    char b[16];
    snprintf(b, sizeof(b), "%04d-%02d-%02d", y, m, d);
    return b;
}
inline long toDay(const std::string& s) {   // expects a valid YYYY-MM-DD
    return daysFromCivil(std::stoi(s.substr(0, 4)), std::stoi(s.substr(5, 2)), std::stoi(s.substr(8, 2)));
}
inline bool validDate(const std::string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (size_t i = 0; i < s.size(); ++i)
        if (i != 4 && i != 7 && !std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    int m = std::stoi(s.substr(5, 2));
    if (m < 1 || m > 12) return false;
    return fromDay(toDay(s)) == s;           // round trip rejects 30 Feb, 31 Apr, etc.
}
inline long todayDay() { return static_cast<long>(std::time(nullptr)) / 86400; }
inline std::string today() { return fromDay(todayDay()); }
inline const char* weekdayName(long day) {
    static const char* n[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    return n[((day + 4) % 7 + 7) % 7];       // 1970-01-01 was a Thursday
}

// ---------------------------------------------------------------- Text helpers
inline std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}
inline std::string bar(double pct, int width = 30) {
    int f = static_cast<int>(std::max(0.0, std::min(100.0, pct)) / 100.0 * width + 0.5);
    return "[" + std::string(f, '#') + std::string(width - f, '-') + "]";
}

}  // namespace eco
