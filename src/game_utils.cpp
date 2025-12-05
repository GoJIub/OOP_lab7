#include "game_utils.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unordered_set>
#include <random>
#include <algorithm>
#include <sstream>

// ---------------- Константы FileObserver ----------------
const int FileObserver::W1 = 18; // имя атакующего
const int FileObserver::W2 = 10; // тип атакующего
const int FileObserver::WP = 11; // позиция атакующего
const int FileObserver::WA = 10; // действие
const int FileObserver::W3 = 18; // имя защищающегося
const int FileObserver::W4 = 10; // тип защищающегося
const int FileObserver::WP2 = 9; // позиция защищающегося

// ---------------- Наблюдатели ----------------
void ConsoleObserver::on_fight(const std::shared_ptr<NPC> &attacker,
                               const std::shared_ptr<NPC> &defender,
                               bool win)
{
    if (!win) return;
    std::cout << ">>> " << attacker->name << " (" << type_to_string(attacker->type) << ")"
              << "  killed  " << defender->name << " (" << type_to_string(defender->type) << ")\n";
}

FileObserver::FileObserver(const std::string &filename) : fname(filename) {
    std::ofstream f(fname, std::ios::trunc);
    if (!f.good()) return;

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

void FileObserver::on_fight(const std::shared_ptr<NPC> &attacker,
                            const std::shared_ptr<NPC> &defender,
                            bool win)
{
    if (!win) return;
    if (!attacker || !defender) return;

    std::ofstream f(fname, std::ios::app);
    if (!f.good()) return;

    std::ostringstream pos_str;
    pos_str << '(' << attacker->x << ',' << attacker->y << ')';
    std::string aPos = pos_str.str();
    pos_str.str("");
    pos_str << '(' << defender->x << ',' << defender->y << ')';
    std::string dPos = pos_str.str();

    f << std::left
      << std::setw(W1) << attacker->name
      << std::setw(W2) << type_to_string(attacker->type)
      << std::setw(WP) << aPos
      << std::setw(WA) << "killed"
      << std::setw(W3) << defender->name
      << std::setw(W4) << type_to_string(defender->type)
      << std::setw(WP2) << dPos
      << "\n";
}

// ---------------- Логика боя ----------------
AttackVisitor::AttackVisitor(const std::shared_ptr<NPC> &attacker_) : attacker(attacker_) {}

FightOutcome AttackVisitor::visit(Orc &defender) {
    return compute(defender);
}

FightOutcome AttackVisitor::visit(Squirrel &defender) {
    return compute(defender);
}

FightOutcome AttackVisitor::visit(Bear &defender) {
    return compute(defender);
}

FightOutcome AttackVisitor::compute(NPC &defender) {
    FightOutcome out;
    NPCType a = attacker->type;
    NPCType d = defender.type;
    bool aKills = kills(a, d);
    bool dKills = kills(d, a);
    out.defenderDead = aKills;
    out.attackerDead = dKills;
    return out;
}

// ---------------- Сохранение/Загрузка ----------------
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
    const int W1 = 18, W2 = 10, W3 = 6, W4 = 6;
    std::cout << std::left
              << std::setw(W1) << "Name"
              << std::setw(W2) << "Type"
              << std::setw(W3) << "X"
              << std::setw(W4) << "Y"
              << "\n";
    std::cout << std::string(W1 + W2 + W3 + W4, '-') << "\n";
    for (auto &p : list) {
        if (!p) continue;
        std::cout << std::left
                  << std::setw(W1) << p->name
                  << std::setw(W2) << type_to_string(p->type)
                  << std::setw(W3) << p->x
                  << std::setw(W4) << p->y
                  << "\n";
    }
    std::cout << std::string(40, '=') << std::endl << std::endl;
}

// ---------------- Раунд боя ----------------
static void mark_dead(std::shared_ptr<NPC> p,
                      std::vector<std::shared_ptr<NPC>> &dead,
                      std::unordered_set<NPC*> &dead_set) 
{
    if (!p) return;
    if (dead_set.insert(p.get()).second)
        dead.push_back(p);
}

std::vector<std::shared_ptr<NPC>> fight_round(std::vector<std::shared_ptr<NPC>> &list, int distance) {
    std::vector<std::shared_ptr<NPC>> dead;
    std::unordered_set<NPC*> dead_set;

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
                mark_dead(B, dead, dead_set);
                A->notify_kill(B);
            }
            if (resB.defenderDead) {
                mark_dead(A, dead, dead_set);
                B->notify_kill(A);
            }
        }
    }

    return dead;
}

// ---------------- Функции рандома (можно заменить на обычный rand) ----------------
NPCType random_type() {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, 3);
    return static_cast<NPCType>(dist(gen));
}

int random_coord(int min, int max) {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}
