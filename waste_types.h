// waste_types.h - waste category class hierarchy (inheritance + polymorphism) and item classifier
#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "utils.h"

namespace eco {

inline const char* const BIN[] = {"", "Blue (Dry)", "Green (Wet)", "Red (Special)", "Black (Reject)"};
inline constexpr int NTYPES = 8;

class WasteType {
    std::string name_, tip_;
    int bin_;
    bool recyclable_;
    double co2_;                       // indicative kg CO2e saved per kg diverted from landfill
public:
    WasteType(std::string n, int b, bool r, double c, std::string t)
        : name_(std::move(n)), tip_(std::move(t)), bin_(b), recyclable_(r), co2_(c) {}
    virtual ~WasteType() = default;
    const std::string& name() const { return name_; }
    const std::string& tip() const { return tip_; }
    int bin() const { return bin_; }
    bool recyclable() const { return recyclable_; }
    double co2PerKg() const { return co2_; }
    virtual std::string disposal() const {          // overridden by special categories
        return std::string("Place in the ") + BIN[bin_] + " bin." + (recyclable_ ? " Recyclable." : " Not recyclable.");
    }
};

struct Plastic : WasteType { Plastic() : WasteType("Plastic", 1, true, 1.5, "Rinse and crush bottles; avoid single-use plastic.") {} };
struct Paper   : WasteType { Paper()   : WasteType("Paper", 1, true, 1.0, "Keep paper dry and clean; reuse the blank side first.") {} };
struct Glass   : WasteType { Glass()   : WasteType("Glass", 1, true, 0.3, "Reuse jars; wrap broken glass before binning.") {} };
struct Metal   : WasteType { Metal()   : WasteType("Metal", 1, true, 4.0, "Metals recycle endlessly; rinse cans before binning.") {} };
struct Organic : WasteType {
    Organic() : WasteType("Organic", 2, true, 0.2, "Compost kitchen waste at home; keep it free of plastic.") {}
    std::string disposal() const override { return "Place in the Green (Wet) bin or compost it. Keep plastic out."; }
};
struct EWaste : WasteType {
    EWaste() : WasteType("E-Waste", 3, true, 2.0, "Donate working devices; give old ones to authorised collectors.") {}
    std::string disposal() const override { return "Never bin it. Hand over at an authorised e-waste collection point."; }
};
struct Hazardous : WasteType {
    Hazardous() : WasteType("Hazardous", 3, false, 0.0, "Choose low-toxicity products; never mix with other waste.") {}
    std::string disposal() const override { return "Seal it separately and hand over at a hazardous-waste collection point."; }
};
struct General : WasteType {
    General() : WasteType("General", 4, false, 0.0, "Reduce reject waste by choosing reusable products.") {}
    std::string disposal() const override { return "Wrap it and place in the Black (Reject) bin. Try to reduce this waste."; }
};

inline const std::vector<std::unique_ptr<WasteType>>& types() {
    static const std::vector<std::unique_ptr<WasteType>> t = [] {
        std::vector<std::unique_ptr<WasteType>> v;
        v.push_back(std::make_unique<Plastic>());   v.push_back(std::make_unique<Paper>());
        v.push_back(std::make_unique<Glass>());     v.push_back(std::make_unique<Metal>());
        v.push_back(std::make_unique<Organic>());   v.push_back(std::make_unique<EWaste>());
        v.push_back(std::make_unique<Hazardous>()); v.push_back(std::make_unique<General>());
        return v;
    }();
    return t;
}

// keyword -> type index; matched as whole words (plural tolerant)
inline const std::vector<std::pair<std::string, int>>& keywords() {
    static const std::vector<std::pair<std::string, int>> k = {
        {"bottle", 0}, {"bag", 0}, {"wrapper", 0}, {"container", 0}, {"straw", 0}, {"plastic", 0},
        {"newspaper", 1}, {"cardboard", 1}, {"book", 1}, {"carton", 1}, {"notebook", 1}, {"paper", 1},
        {"jar", 2}, {"glass", 2}, {"mirror", 2},
        {"can", 3}, {"tin", 3}, {"foil", 3}, {"steel", 3}, {"aluminium", 3}, {"metal", 3},
        {"peel", 4}, {"food", 4}, {"vegetable", 4}, {"leaf", 4}, {"leaves", 4}, {"tea", 4}, {"fruit", 4},
        {"phone", 5}, {"charger", 5}, {"battery", 5}, {"laptop", 5}, {"earphone", 5}, {"cable", 5},
        {"paint", 6}, {"medicine", 6}, {"syringe", 6}, {"pesticide", 6}, {"bulb", 6}, {"tubelight", 6},
        {"diaper", 7}, {"tissue", 7}, {"sanitary", 7}, {"cigarette", 7}};
    return k;
}

inline int classify(const std::string& text) {
    std::string q = " ";
    for (char c : lower(text)) q += std::isalnum(static_cast<unsigned char>(c)) ? c : ' ';
    q += " ";
    for (const auto& kv : keywords())
        if (q.find(" " + kv.first + " ") != std::string::npos || q.find(" " + kv.first + "s ") != std::string::npos)
            return kv.second;
    return -1;
}

}  // namespace eco
