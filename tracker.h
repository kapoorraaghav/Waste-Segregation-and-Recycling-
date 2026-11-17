// tracker.h - Tracker class: records, persistence, reports, demo data
#pragma once
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "analytics.h"

namespace eco {

class Tracker {
    std::vector<Entry> v;
    int nextId = 1;
    double goal = 60;                  // target recycling rate (%)
    std::string dataFile, cfgFile;

public:
    Tracker(std::string d, std::string c) : dataFile(std::move(d)), cfgFile(std::move(c)) { load(); }

    // ------------------------------------------------------------ operations
    void add() {
        Entry e;
        e.id = nextId;
        promptEntry(e);
        v.push_back(e);
        ++nextId;
        save();
        const auto& t = *types()[e.type];
        if (e.correct()) std::cout << "\nCorrect bin! " << t.disposal() << "\n";
        else std::cout << "\nWrong bin! " << t.name() << " waste: " << t.disposal() << "\n";
        std::cout << "Eco-points earned: +" << pointsFor(e) << "\n";
    }

    void list(std::ostream& os, int mode = 1) const {
        if (v.empty()) { os << "\nNo entries yet.\n"; return; }
        std::vector<Entry> s = v;
        if (mode == 2) std::sort(s.begin(), s.end(), [](const Entry& a, const Entry& b) { return a.kg > b.kg; });
        else if (mode == 3) std::sort(s.begin(), s.end(), [](const Entry& a, const Entry& b) { return a.type < b.type; });
        else std::sort(s.begin(), s.end());                      // uses Entry::operator<
        os << "\n" << std::left << std::setw(5) << "ID" << std::setw(12) << "Date" << std::setw(20) << "Item"
           << std::setw(11) << "Category" << std::setw(9) << "Kg" << std::setw(15) << "Bin used" << "OK?\n"
           << std::string(76, '-') << "\n";
        for (const auto& e : s) os << e << "\n";                 // uses operator<<
    }

    void report(std::ostream& os, const std::string& month = "") const {
        Stats s = compute(v, month);
        if (s.n == 0) { os << "\nNo data" << (month.empty() ? "" : " for " + month) << ".\n"; return; }
        os << "\n=========== DASHBOARD" << (month.empty() ? "" : " (" + month + ")") << " ===========\n";
        for (const auto& kv : s.byType) {
            double p = 100.0 * kv.second / s.total;
            os << std::left << std::setw(11) << types()[kv.first]->name() << std::right << std::setw(7) << std::fixed
               << std::setprecision(2) << kv.second << " kg " << std::setw(6) << std::setprecision(1) << p << "%  "
               << std::string(static_cast<size_t>(p / 4), '#') << "\n";
        }
        os << std::string(46, '-') << "\n"
           << "Total waste         : " << std::setprecision(2) << s.total << " kg (" << s.n << " entries)\n"
           << "Recycled/composted  : " << s.recycled << " kg\n"
           << "Recycling rate      : " << std::setprecision(1) << s.rate() << " %   goal " << goal << " %\n"
           << "  " << bar(s.rate()) << (s.rate() >= goal ? "  GOAL MET" : "") << "\n"
           << "Segregation accuracy: " << s.accuracy() << " %  (" << s.correct << "/" << s.n << " in correct bin)\n"
           << "  " << bar(s.accuracy()) << "\n"
           << "Est. CO2e saved     : " << std::setprecision(2) << s.co2 << " kg (indicative)\n"
           << "Rating              : " << rating(s.rate()) << "\n";
        if (!s.wrong.empty()) {
            os << "\nTop segregation mistakes:\n";
            std::vector<std::pair<int, std::pair<int, int>>> w;
            for (const auto& kv : s.wrong) w.push_back({kv.second, kv.first});
            std::sort(w.rbegin(), w.rend());
            for (size_t i = 0; i < w.size() && i < 3; ++i)
                os << "  " << types()[w[i].second.first]->name() << " put in " << BIN[w[i].second.second] << " x" << w[i].first
                   << "  (should be " << BIN[types()[w[i].second.first]->bin()] << ")\n";
        }
        auto top = std::max_element(s.byType.begin(), s.byType.end(),
                                    [](const auto& a, const auto& b) { return a.second < b.second; });
        os << "\nTip (your biggest category, " << types()[top->first]->name() << "): " << types()[top->first]->tip() << "\n";
    }

    void monthly() const {
        if (v.empty()) { std::cout << "\nNo data.\n"; return; }
        std::map<std::string, int> months;
        for (const auto& e : v) months[e.date.substr(0, 7)];
        std::cout << "\nMonth     Total(kg) Recycled Rate%   Accuracy%  Trend\n" << std::string(56, '-') << "\n";
        double prev = -1;
        for (const auto& kv : months) {
            Stats s = compute(v, kv.first);
            std::cout << std::left << std::setw(10) << kv.first << std::setw(10) << std::fixed << std::setprecision(2)
                      << s.total << std::setw(9) << s.recycled << std::setw(8) << std::setprecision(1) << s.rate()
                      << std::setw(11) << s.accuracy();
            if (prev < 0) std::cout << "-"; else std::cout << std::showpos << s.rate() - prev << std::noshowpos;
            std::cout << "\n";
            prev = s.rate();
        }
    }

    void weekly() const { printWeeklyChart(std::cout, v); }
    void ecoProfileView() const { printEcoProfile(std::cout, v); }

    void search() const {
        std::string q = lower(readLine("Search item name: "));
        bool found = false;
        for (const auto& e : v)
            if (lower(e.item).find(q) != std::string::npos) {
                std::cout << "  #" << e.id << "  " << e.date << "  " << e.item << "  (" << types()[e.type]->name() << ", "
                          << std::fixed << std::setprecision(2) << e.kg << " kg)\n";
                found = true;
            }
        if (!found) std::cout << "  No matches.\n";
    }

    void edit() {
        Entry* e = find();
        Entry copy = *e;                  // edit a copy so a failed input leaves the record untouched
        promptEntry(copy);
        *e = copy;
        save();
        std::cout << "  Updated.\n";
    }

    void del() {
        int id = find()->id;
        v.erase(std::remove_if(v.begin(), v.end(), [id](const Entry& x) { return x.id == id; }), v.end());
        save();
        std::cout << "  Deleted.\n";
    }

    void setGoal() {
        goal = readValue<double>("New recycling goal in % (1-100): ", 1.0, 100.0);
        std::ofstream(cfgFile) << goal << '\n';
        std::cout << "  Goal set to " << goal << " %.\n";
    }

    void exportReport() const {
        std::ofstream out("waste_report.txt");
        if (!out) throw DataError("Cannot write waste_report.txt");
        out << "WASTE SEGREGATION & RECYCLING TRACKER - REPORT (" << today() << ")\n";
        report(out);
        printEcoProfile(out, v);
        printWeeklyChart(out, v);
        list(out);
        std::cout << "  Report saved to waste_report.txt\n";
    }

    void loadDemo() {
        struct D { int ago; const char* item; int type; double kg; int bin; };
        static const D demo[] = {
            {14, "Plastic bottle", 0, 0.40, 1}, {14, "Banana peel", 4, 0.90, 2},
            {13, "Newspaper", 1, 0.60, 1},      {13, "Tea leaves", 4, 0.50, 2},    {13, "Pen refill", 7, 0.10, 4},
            {12, "Glass jar", 2, 0.80, 1},      {12, "Plastic bag", 0, 0.20, 4},
            {11, "Steel can", 3, 0.30, 1},      {11, "Vegetable waste", 4, 1.10, 2},
            {10, "Old charger", 5, 0.25, 3},    {10, "Cardboard box", 1, 1.00, 1},
            {9, "Paint can", 6, 0.70, 3},       {9, "Food waste", 4, 0.80, 2},     {9, "Plastic wrapper", 0, 0.15, 4},
            {8, "Milk carton", 1, 0.30, 1},     {8, "Aluminium foil", 3, 0.10, 1},
            {7, "Plastic container", 0, 0.35, 1}, {7, "Fruit peel", 4, 0.60, 2},
            {6, "Notebook", 1, 0.50, 1},        {6, "Battery", 5, 0.10, 3},        {6, "Tissue", 7, 0.20, 4},
            {5, "Glass bottle", 2, 0.60, 1},    {5, "Vegetable peel", 4, 0.70, 2},
            {4, "Plastic straw", 0, 0.05, 1},   {4, "Newspaper", 1, 0.70, 1},
            {3, "Tin can", 3, 0.25, 1},         {3, "Tea leaves", 4, 0.40, 2},     {3, "Diaper", 7, 0.40, 4},
            {2, "Cardboard", 1, 0.90, 1},       {2, "Plastic bottle", 0, 0.30, 1},
            {1, "Food waste", 4, 1.00, 2},      {1, "Old earphones", 5, 0.10, 3},
            {0, "Plastic bag", 0, 0.20, 1},     {0, "Banana peel", 4, 0.50, 2}};
        long t0 = todayDay();
        for (const auto& d : demo) {
            Entry e;
            e.id = nextId++; e.date = fromDay(t0 - d.ago); e.item = d.item; e.type = d.type; e.kg = d.kg; e.binUsed = d.bin;
            v.push_back(e);
        }
        save();
        std::cout << "  Added " << sizeof(demo) / sizeof(demo[0]) << " demo entries covering the last 15 days.\n";
    }

    void clearAll() {
        std::string a = readLine("This deletes ALL entries. Type YES to confirm: ");
        if (a != "YES") { std::cout << "  Cancelled.\n"; return; }
        v.clear();
        nextId = 1;
        save();
        std::cout << "  All data cleared.\n";
    }

    static void whichBin() {
        int t = classify(readLine("Describe the item (e.g. plastic bottle, banana peel): "));
        if (t < 0) { std::cout << "  Not sure. Treat as General waste or check local guidelines.\n"; return; }
        std::cout << "  -> " << types()[t]->name() << " waste. " << types()[t]->disposal() << "\n";
    }

    // Parses one CSV record; throws DataError on any problem.
    static Entry parseLine(const std::string& line) {
        std::stringstream ss(line);
        std::string id, date, ty, kg, bin, item;
        if (!std::getline(ss, id, ',') || !std::getline(ss, date, ',') || !std::getline(ss, ty, ',') ||
            !std::getline(ss, kg, ',') || !std::getline(ss, bin, ',') || !std::getline(ss, item))
            throw DataError("Malformed record");
        Entry e;
        try {
            e.id = std::stoi(id); e.date = date; e.type = std::stoi(ty); e.kg = std::stod(kg); e.binUsed = std::stoi(bin);
        } catch (const std::exception&) { throw DataError("Non-numeric field"); }
        e.item = item;
        if (e.type < 0 || e.type >= NTYPES || e.binUsed < 1 || e.binUsed > 4 || e.kg <= 0 || !validDate(e.date))
            throw DataError("Field out of range");
        return e;
    }

    const std::vector<Entry>& entries() const { return v; }

private:
    void promptEntry(Entry& e) {
        e.item = readLine("Item name: ");
        int guess = classify(e.item);
        std::cout << "\nCategories:\n";
        for (int i = 0; i < NTYPES; ++i)
            std::cout << "  " << i + 1 << ". " << std::setw(10) << std::left << types()[i]->name() << " -> "
                      << BIN[types()[i]->bin()] << "\n";
        if (guess >= 0) std::cout << "  (Suggestion: " << guess + 1 << " - " << types()[guess]->name() << ")\n";
        e.type = readValue<int>("Choose category (1-8): ", 1, NTYPES) - 1;
        e.kg = readValue<double>("Weight in kg: ", 0.001, 10000.0);
        std::cout << "\nBins: 1 Blue(Dry)  2 Green(Wet)  3 Red(Special)  4 Black(Reject)\n";
        e.binUsed = readValue<int>("Which bin did you actually use? (1-4): ", 1, 4);
        while (true) {
            std::cout << "Date YYYY-MM-DD (Enter for today): ";
            std::string d;
            if (!std::getline(std::cin, d)) throw InputClosedError();
            if (d.empty()) { e.date = today(); break; }
            if (validDate(d)) { e.date = d; break; }
            std::cout << "  Invalid date.\n";
        }
    }

    Entry* find() {
        if (v.empty()) throw TrackerError("No entries yet");
        int id = readValue<int>("Enter entry ID: ", 1, 1000000);
        for (auto& e : v) if (e.id == id) return &e;
        throw EntryNotFoundError(id);
    }

    void save() const {
        std::ofstream out(dataFile);
        if (!out) throw DataError("Cannot write " + dataFile);
        for (const auto& e : v)
            out << e.id << ',' << e.date << ',' << e.type << ',' << e.kg << ',' << e.binUsed << ',' << e.item << '\n';
    }

    void load() {
        std::ifstream cfg(cfgFile);
        double g;
        if (cfg >> g && g >= 1 && g <= 100) goal = g;
        std::ifstream in(dataFile);
        std::string line;
        int skipped = 0;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            try {
                Entry e = parseLine(line);
                v.push_back(e);
                nextId = std::max(nextId, e.id + 1);
            } catch (const DataError&) { ++skipped; }
        }
        if (skipped) std::cout << "Warning: " << skipped << " corrupt record(s) in " << dataFile << " were skipped.\n";
    }
};

}  // namespace eco
