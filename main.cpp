#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include <memory>
#include <unordered_set>
#include <iomanip>

#include "include/npc.h"
#include "include/orc.h"
#include "include/squirrel.h"
#include "include/bear.h"

// ---------------- Наблюдатели ----------------
class ConsoleObserver : public IFightObserver {
public:
    void on_fight(const std::shared_ptr<NPC> &attacker,
                  const std::shared_ptr<NPC> &defender,
                  bool win) override {
        if (!win) return;
        std::cout << ">>> " << attacker->name << " (" << type_to_string(attacker->type) << ")"
                  << "  killed  " << defender->name << " (" << type_to_string(defender->type) << ")\n";
    }
};

class FileObserver : public IFightObserver {
public:
        const int W1 = 18; // имя атакующего
        const int W2 = 10; // тип атакующего
        const int WP = 11; // позиция атакующего
        const int WA = 10; // действие
        const int W3 = 18; // имя защищающегося
        const int W4 = 10; // тип защищающегося
        const int WP2 = 9; // позиция защищающегося

    FileObserver(const std::string &filename) : fname(filename) {
        std::ofstream f(fname, std::ios::trunc);
        if (f.good()) {
            f << std::left
              << std::setw(W1)  << "Attacker"
              << std::setw(W2)  << "Type"
              << std::setw(WP)  << "Pos"
              << std::setw(WA)  << "Action"
              << std::setw(W3)  << "Defender"
              << std::setw(W4)  << "Type"
              << std::setw(WP2) << "Pos"
              << "\n";

            f << std::string(W1 + W2 + WP + WA + W3 + W4 + WP2, '-') << "\n";
        }
    }

    void on_fight(const std::shared_ptr<NPC> &attacker,
                  const std::shared_ptr<NPC> &defender,
                  bool win) override 
    {
        if (!win) return;
        if (!attacker || !defender) return;

        std::ofstream f(fname, std::ios::app);
        if (!f.good()) return;
        
        auto pos_str = [](int x, int y) {
            std::ostringstream os;
            os << '(' << x << ',' << y << ')';
            return os.str();
        };

        f << std::left
          << std::setw(W1)  << attacker->name
          << std::setw(W2)  << type_to_string(attacker->type)
          << std::setw(WP)  << pos_str(attacker->x, attacker->y)
          << std::setw(WA)  << "killed"
          << std::setw(W3)  << defender->name
          << std::setw(W4)  << type_to_string(defender->type)
          << std::setw(WP2) << pos_str(defender->x, defender->y)
          << "\n";
    }

private:
    std::string fname;
};

// ---------------- Логика боя ----------------
class AttackVisitor : public IFightVisitor {
public:
    AttackVisitor(const std::shared_ptr<NPC> &attacker_) : attacker(attacker_) {}
    FightOutcome visit(Orc &defender) override {
        return compute(defender);
    }
    FightOutcome visit(Squirrel &defender) override {
        return compute(defender);
    }
    FightOutcome visit(Bear &defender) override {
        return compute(defender);
    }
private:
    std::shared_ptr<NPC> attacker;

    FightOutcome compute(NPC &defender) {
        FightOutcome out;
        NPCType a = attacker->type;
        NPCType d = defender.type;
        bool aKills = kills(a, d);
        bool dKills = kills(d, a);
        out.defenderDead = aKills;
        out.attackerDead = dKills;
        return out;
    }
};

// ---------------- Сохранение и загрузка ----------------
void save_all(const std::vector<std::shared_ptr<NPC>> &list, const std::string &filename) {
    std::ofstream os(filename, std::ios::trunc);
    os << list.size() << '\n';
    for (auto &p : list) p->save(os);
}

std::vector<std::shared_ptr<NPC>> load_all(const std::string &filename) {
    std::vector<std::shared_ptr<NPC>> res;
    std::ifstream is(filename);
    if (!is.good()) return res;
    size_t cnt = 0;
    is >> cnt;
    for (size_t i = 0; i < cnt; ++i) {
        auto p = createNPCFromStream(is);
        if (p) res.push_back(p);
    }
    return res;
}

void print_all(const std::vector<std::shared_ptr<NPC>> &list) {
    std::cout << "\n=== NPCs (" << list.size() << ") ===\n";
    const int W1 = 18; // имя
    const int W2 = 10; // тип
    const int W3 = 6;  // x
    const int W4 = 6;  // y
    std::cout << std::left << std::setw(W1) << "Name" 
              << std::setw(W2) << "Type" 
              << std::setw(W3) << "X" 
              << std::setw(W4) << "Y" << "\n";
    std::cout << std::string(W1 + W2 + W3 + W4, '-') << "\n";
    for (auto &p : list) {
        if (!p) continue;
        std::cout << std::left << std::setw(W1) << p->name
                  << std::setw(W2) << type_to_string(p->type)
                  << std::setw(W3) << p->x
                  << std::setw(W4) << p->y << "\n";
    }
    std::cout << std::string(40, '=') << std::endl << std::endl;
}

// ---------------- Сам бой ----------------
std::vector<std::shared_ptr<NPC>> fight_round(std::vector<std::shared_ptr<NPC>> &list, int distance) {
    std::vector<std::shared_ptr<NPC>> dead;
    std::unordered_set<NPC*> dead_set;

    auto mark_dead = [&](const std::shared_ptr<NPC>& p){
        if (!p) return;
        if (dead_set.insert(p.get()).second)
            dead.push_back(p);
    };

    const size_t n = list.size();
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            auto &A = list[i];
            auto &B = list[j];
            if (!A || !B) continue;
            if (dead_set.count(A.get()) || dead_set.count(B.get())) continue;
            if (!A->is_close(B, distance)) continue;

            AttackVisitor vA(A);
            FightOutcome resA = B->accept(vA);
            AttackVisitor vB(B);
            FightOutcome resB = A->accept(vB);

            if (resA.defenderDead) {
                mark_dead(B);
                A->notify_kill(B);
            }
            if (resB.defenderDead) {
                mark_dead(A);
                B->notify_kill(A);
            }
        }
    }

    return dead;
}

// ---------------- Main ----------------
inline NPCType random_type() {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, 3);
    return static_cast<NPCType>(dist(gen));
}

inline int random_coord(int min = 0, int max = 500) {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

int main() {
    auto consoleObs = std::make_shared<ConsoleObserver>();
    auto fileObs = std::make_shared<FileObserver>("log.txt");

    std::vector<std::shared_ptr<NPC>> list;
    const int START = 100;
    for (int i = 0; i < START; ++i) {
        NPCType t = random_type();
        std::string name;
        if (t == NPCType::Orc) name = "Orc_" + std::to_string(i+1);
        else if (t == NPCType::Bear) name = "Bear_" + std::to_string(i+1);
        else name = "Squirrel_" + std::to_string(i+1);

        int x = random_coord();
        int y = random_coord();
        auto p = createNPC(t, name, x, y);
        if (p) {
            p->subscribe(consoleObs);
            p->subscribe(fileObs);
            list.push_back(p);
        }
    }

    print_all(list);
    save_all(list, "npcs.txt");
    std::cout << "Saved to npcs.txt\n";

    auto loaded = load_all("npcs.txt");
    for (auto &p : loaded) {
        p->subscribe(consoleObs);
        p->subscribe(fileObs);
    }
    std::cout << "Loaded " << loaded.size() << " NPCs\n";

    int total_killed = 0;
    for (int distance = 20; distance <= 200 && !loaded.empty(); distance += 20) {
        std::cout << "\n";
        auto dead = fight_round(loaded, distance);
        for (auto &d : dead) {
            auto it = std::find(loaded.begin(), loaded.end(), d);
            if (it != loaded.end()) loaded.erase(it);
        }
        std::cout << "Distance=" << distance << " killed=" << dead.size() << " remaining=" << loaded.size() << "\n";
        total_killed += (int)dead.size();
    }

    std::cout << "Fight ended. Total killed: " << total_killed << "\n";
    print_all(loaded);
    return 0;
}
