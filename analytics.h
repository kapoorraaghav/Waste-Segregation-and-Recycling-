// analytics.h - Entry record (operator overloading), statistics, eco-points / streaks / badges, weekly chart
#pragma once
#include <iomanip>
#include <map>
#include <ostream>
#include <string>
#include <vector>
#include "waste_types.h"

namespace eco {

struct Entry {
    int id = 0;
    std::string date, item;
    int type = 7;            // index into types()
    double kg = 0;
    int binUsed = 4;         // bin the user actually used (1-4)
    bool correct() const { return binUsed == types()[type]->bin(); }
    bool operator<(const Entry& o) const { return date < o.date || (date == o.date && id < o.id); }
};

inline std::ostream& operator<<(std::ostream& os, const Entry& e) {      // one table row
    os << std::left << std::setw(5) << e.id << std::setw(12) << e.date << std::setw(20) << e.item.substr(0, 18)
       << std::setw(11) << types()[e.type]->name() << std::setw(9) << std::fixed << std::setprecision(2) << e.kg
       << std::setw(15) << BIN[e.binUsed] << (e.correct() ? "Yes" : "NO");
    return os;
}

// ---------------------------------------------------------------- Statistics
struct Stats {
    double total = 0, recycled = 0, co2 = 0;
    int n = 0, correct = 0;
    std::map<int, double> byType;
    std::map<std::pair<int, int>, int> wrong;       // (type, bin used) -> count
    double rate() const { return total > 0 ? 100.0 * recycled / total : 0; }
    double accuracy() const { return n > 0 ? 100.0 * correct / n : 0; }
};

inline Stats compute(const std::vector<Entry>& v, const std::string& month = "") {
    Stats s;
    for (const auto& e : v) {
        if (!month.empty() && e.date.substr(0, 7) != month) continue;
        const auto& t = *types()[e.type];
        s.n++;
        s.total += e.kg;
        s.byType[e.type] += e.kg;
        if (t.recyclable()) { s.recycled += e.kg; s.co2 += e.kg * t.co2PerKg(); }
        if (e.correct()) s.correct++; else s.wrong[{e.type, e.binUsed}]++;
    }
    return s;
}

inline const char* rating(double r) {
    if (r >= 75) return "Excellent - Eco Champion";
    if (r >= 50) return "Good - Keep going";
    if (r >= 25) return "Fair - Room to improve";
    return "Poor - Segregate more";
}

// ---------------------------------------------------------------- Gamification
struct Badge { int at; const char* name; };
inline const std::vector<Badge>& badges() {
    static const std::vector<Badge> b = {{0, "Seedling"}, {50, "Bronze Recycler"}, {150, "Silver Recycler"},
                                         {300, "Gold Recycler"}, {600, "Platinum Eco Hero"}};
    return b;
}
inline size_t badgeIndex(int points) {
    size_t idx = 0;
    for (size_t i = 0; i < badges().size(); ++i) if (points >= badges()[i].at) idx = i;
    return idx;
}

// +10 for using the correct bin, plus 5 per kg when the correct bin was used for recyclable waste
inline int pointsFor(const Entry& e) {
    if (!e.correct()) return 0;
    int p = 10;
    if (types()[e.type]->recyclable()) p += static_cast<int>(e.kg * 5 + 0.5);
    return p;
}

struct EcoProfile { int points = 0, currentStreak = 0, bestStreak = 0; };

// streak = consecutive calendar days on which every logged item went in the correct bin
inline EcoProfile ecoProfile(const std::vector<Entry>& v) {
    EcoProfile p;
    std::map<long, bool> clean;
    for (const auto& e : v) {
        p.points += pointsFor(e);
        long d = toDay(e.date);
        auto it = clean.find(d);
        if (it == clean.end()) clean[d] = e.correct(); else it->second = it->second && e.correct();
    }
    int run = 0;
    long lastDay = 0;
    bool lastClean = false;
    for (const auto& kv : clean) {
        if (kv.second) run = (lastClean && kv.first == lastDay + 1) ? run + 1 : 1;
        else run = 0;
        lastDay = kv.first;
        lastClean = kv.second;
        p.bestStreak = std::max(p.bestStreak, run);
    }
    p.currentStreak = run;
    return p;
}

inline void printEcoProfile(std::ostream& os, const std::vector<Entry>& v) {
    if (v.empty()) { os << "\nNo data yet - log some waste to earn eco-points.\n"; return; }
    EcoProfile p = ecoProfile(v);
    size_t i = badgeIndex(p.points);
    os << "\n=========== ECO PROFILE ===========\n"
       << "Eco-points     : " << p.points << "\n"
       << "Badge          : " << badges()[i].name << "\n";
    if (i + 1 < badges().size()) {
        int lo = badges()[i].at, hi = badges()[i + 1].at;
        os << "Next badge     : " << badges()[i + 1].name << " at " << hi << " pts (" << hi - p.points << " to go)\n"
           << "  " << bar(100.0 * (p.points - lo) / (hi - lo)) << "\n";
    } else {
        os << "Top badge reached!\n";
    }
    os << "Current streak : " << p.currentStreak << " day(s) of 100% correct segregation\n"
       << "Best streak    : " << p.bestStreak << " day(s)\n"
       << "Scoring        : +10 per correct bin, +5 per kg of recyclable waste binned correctly\n";
}

// ---------------------------------------------------------------- Weekly ASCII chart
inline void printWeeklyChart(std::ostream& os, const std::vector<Entry>& v) {
    if (v.empty()) { os << "\nNo data.\n"; return; }
    long end = 0;
    for (const auto& e : v) end = std::max(end, toDay(e.date));
    std::map<long, std::pair<double, double>> day;       // day -> (recycled kg, other kg)
    double mx = 0;
    for (const auto& e : v) {
        long d = toDay(e.date);
        if (d < end - 6) continue;
        (types()[e.type]->recyclable() ? day[d].first : day[d].second) += e.kg;
        mx = std::max(mx, day[d].first + day[d].second);
    }
    os << "\n===== WEEKLY WASTE (7 days up to " << fromDay(end) << ") =====\n"
       << "  # recycled/composted   . non-recyclable\n";
    for (long d = end - 6; d <= end; ++d) {
        auto it = day.find(d);
        double r = it == day.end() ? 0 : it->second.first, o = it == day.end() ? 0 : it->second.second;
        int rw = mx > 0 ? static_cast<int>(r / mx * 30 + 0.5) : 0, ow = mx > 0 ? static_cast<int>(o / mx * 30 + 0.5) : 0;
        os << weekdayName(d) << " " << fromDay(d).substr(5) << " |" << std::string(rw, '#') << std::string(ow, '.')
           << " " << std::fixed << std::setprecision(2) << r + o << " kg\n";
    }
}

}  // namespace eco
