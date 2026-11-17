// Waste Segregation and Recycling Tracker (v3)
// C++17 console app: OOP (inheritance, polymorphism), templates, exceptions, operator overloading, STL, file I/O.
// Build : make            (or: g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o waste_tracker)
// Run   : ./waste_tracker            interactive menu
//         ./waste_tracker --test     run built-in self-tests

#include <sstream>
#include "tracker.h"

using namespace eco;

static int runTests() {
    int fail = 0;
    auto check = [&](bool ok, const std::string& name) {
        std::cout << (ok ? "PASS  " : "FAIL  ") << name << "\n";
        if (!ok) ++fail;
    };

    std::vector<Entry> a(3);
    a[0] = {1, "2026-11-01", "bottle", 0, 1.0, 1};      // plastic, correct bin
    a[1] = {2, "2026-11-02", "paint", 6, 1.0, 4};       // hazardous, wrong bin
    a[2] = {3, "2026-10-05", "peel", 4, 2.0, 2};        // organic, correct bin
    Stats s = compute(a);
    check(std::abs(s.total - 4.0) < 1e-9, "total weight");
    check(std::abs(s.rate() - 75.0) < 1e-9, "recycling rate = 75%");
    check(std::abs(s.accuracy() - 200.0 / 3) < 1e-6, "segregation accuracy = 66.7%");
    check(std::abs(s.co2 - 1.9) < 1e-9, "CO2e saved = 1.9 kg");
    check(compute(a, "2026-11").n == 2, "month filter");

    check(classify("Plastic bottles") == 0 && classify("banana peel") == 4 && classify("scan") == -1, "keyword classifier");
    check(types()[5]->disposal().find("Never bin") != std::string::npos, "polymorphic disposal() override");

    check(validDate("2026-11-15") && validDate("2028-02-29") && !validDate("2027-02-29") && !validDate("2026-04-31") &&
              !validDate("2026-13-01") && !validDate("15-11-2026"), "date validation (leap years, month lengths)");
    check(fromDay(toDay("2026-11-15")) == "2026-11-15" && toDay("2026-11-16") - toDay("2026-11-15") == 1, "date round trip");
    check(std::string(weekdayName(toDay("2026-11-15"))) == "Sun", "weekday calculation");

    std::vector<Entry> b = {{1, "2026-11-01", "a", 0, 1.0, 1}, {2, "2026-11-02", "b", 6, 1.0, 4},
                            {3, "2026-11-03", "c", 4, 2.0, 2}, {4, "2026-11-04", "d", 6, 0.5, 3},
                            {5, "2026-11-05", "e", 7, 0.5, 4}};
    EcoProfile p = ecoProfile(b);
    check(p.points == 55, "eco-points = 55");
    check(p.bestStreak == 3 && p.currentStreak == 3, "streak: best 3, current 3");
    check(std::string(badges()[badgeIndex(55)].name) == "Bronze Recycler" && badgeIndex(600) == 4, "badge levels");

    {   // templated reader: skips bad input, returns first valid value
        std::istringstream in("abc\n99\n7\n");
        std::ostringstream out;
        check(readValue<int>(in, out, "n: ", 1, 10) == 7, "readValue<int> re-prompts on bad input");
        std::istringstream in2("2.5\n");
        check(std::abs(readValue<double>(in2, out, "x: ", 0.0, 5.0) - 2.5) < 1e-9, "readValue<double>");
    }
    {   // custom exceptions
        std::istringstream empty("");
        std::ostringstream out;
        bool thrown = false;
        try { readValue<int>(empty, out, "n: ", 1, 5); } catch (const InputClosedError&) { thrown = true; }
        check(thrown, "InputClosedError thrown on EOF");
        bool d1 = false, d2 = false;
        try { Tracker::parseLine("garbage"); } catch (const DataError&) { d1 = true; }
        try { Tracker::parseLine("1,2026-13-01,0,1.0,1,x"); } catch (const DataError&) { d2 = true; }
        check(d1 && d2 && Tracker::parseLine("7,2026-11-15,4,1.5,2,Banana, peel").item == "Banana, peel",
              "DataError on corrupt records; valid record parsed");
    }
    {   // operator overloading
        std::ostringstream os;
        os << b[1];
        std::vector<Entry> c = {b[2], b[0]};
        std::sort(c.begin(), c.end());
        check(os.str().find("NO") != std::string::npos && c[0].id == 1, "Entry operator<< and operator<");
    }
    std::cout << (fail ? "\nSome tests FAILED\n" : "\nAll tests passed\n");
    return fail ? 1 : 0;
}

static void printMenu() {
    std::cout << "\n====== WASTE SEGREGATION & RECYCLING TRACKER ======\n"
              << "  1. Log waste entry        8. Search entries\n"
              << "  2. View entries (sorted)  9. Edit entry\n"
              << "  3. Which bin?            10. Delete entry\n"
              << "  4. Dashboard             11. Set recycling goal\n"
              << "  5. Monthly report        12. Export report to file\n"
              << "  6. Weekly chart          13. Load demo data\n"
              << "  7. Eco profile (points)  14. Clear all data\n"
              << "  0. Exit\n";
}

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--test") return runTests();
    try {
        Tracker t("waste_data.csv", "waste_config.txt");
        while (true) {
            printMenu();
            int choice = readValue<int>("Choice: ", 0, 14);
            try {
                switch (choice) {
                    case 1: t.add(); break;
                    case 2: t.list(std::cout, readValue<int>("Sort by 1) date  2) weight  3) category: ", 1, 3)); break;
                    case 3: Tracker::whichBin(); break;
                    case 4: t.report(std::cout); break;
                    case 5: t.monthly(); break;
                    case 6: t.weekly(); break;
                    case 7: t.ecoProfileView(); break;
                    case 8: t.search(); break;
                    case 9: t.edit(); break;
                    case 10: t.del(); break;
                    case 11: t.setGoal(); break;
                    case 12: t.exportReport(); break;
                    case 13: t.loadDemo(); break;
                    case 14: t.clearAll(); break;
                    case 0: std::cout << "Thank you for keeping the planet clean!\n"; return 0;
                }
            } catch (const InputClosedError&) {
                throw;
            } catch (const TrackerError& e) {
                std::cout << "  Error: " << e.what() << "\n";
            }
        }
    } catch (const InputClosedError&) {
        std::cout << "\nInput closed. Goodbye!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}
