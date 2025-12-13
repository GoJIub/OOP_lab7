#include "../include/game_utils.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unordered_set>
#include <random>
#include <algorithm>
#include <sstream>

#include <thread>
#include <mutex>
#include <chrono>
#include <queue>
#include <optional>
#include <array>
#include <thread>

using namespace std::chrono_literals;
std::mutex print_mutex;

// ---------------- Константы FileObserver ----------------
const int FileObserver::W1 = 18; // имя атакующего
const int FileObserver::W2 = 10; // тип атакующего
const int FileObserver::WP = 11; // позиция атакующего
const int FileObserver::WA = 10; // действие
const int FileObserver::W3 = 18; // имя защищающегося
const int FileObserver::W4 = 10; // тип защищающегося
const int FileObserver::WP2 = 9; // позиция защищающегося


// ---------------- Наблюдатели ----------------
std::shared_ptr<IFightObserver> ConsoleObserver::get() {
    static ConsoleObserver instance;
    return std::shared_ptr<IFightObserver>(&instance, [](IFightObserver*) {});
}

void ConsoleObserver::on_fight(const std::shared_ptr<NPC>& attacker,
                               const std::shared_ptr<NPC>& defender,
                               FightOutcome outcome)
{
    if (!attacker || !defender) return;

    std::lock_guard<std::mutex> lck(print_mutex);

    switch (outcome) {
    case FightOutcome::DefenderKilled:
        std::cout << ">>> "
                  << attacker->name << " (" << type_to_string(attacker->type) << ")"
                  << " killed "
                  << defender->name << " (" << type_to_string(defender->type) << ")\n";
        break;

    case FightOutcome::AttackerKilled:
        std::cout << ">>> "
                  << defender->name << " (" << type_to_string(defender->type) << ")"
                  << " killed "
                  << attacker->name << " (" << type_to_string(attacker->type) << ")\n";
        break;

    case FightOutcome::NobodyDied:
        std::cout << ">>> "
                  << defender->name << " (" << type_to_string(defender->type) << ")"
                  << " escaped from "
                  << attacker->name << " (" << type_to_string(attacker->type) << ")\n";
        break;
    }
}

FileObserver::FileObserver(const std::string& filename) : fname(filename)
{
    std::ofstream f(fname, std::ios::trunc);
    if (!f.good()) return;

    std::lock_guard<std::mutex> lck(print_mutex);

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

std::shared_ptr<IFightObserver> FileObserver::get(const std::string& filename)
{
    static std::shared_ptr<IFightObserver> instance =
        std::shared_ptr<IFightObserver>(
            new FileObserver(filename),
            [](IFightObserver*){}
        );

    return instance;
}

void FileObserver::on_fight(const std::shared_ptr<NPC>& attacker,
                            const std::shared_ptr<NPC>& defender,
                            FightOutcome outcome)
{
    if (!attacker || !defender) return;

    std::ofstream f(fname, std::ios::app);
    if (!f.good()) return;

    std::lock_guard<std::mutex> lck(print_mutex);

    std::ostringstream ss;
    ss << '(' << attacker->x << ',' << attacker->y << ')';
    std::string aPos = ss.str();
    ss.str("");
    ss << '(' << defender->x << ',' << defender->y << ')';
    std::string dPos = ss.str();

    std::string action =
        (outcome == FightOutcome::NobodyDied) ? "escaped" : "killed";

    f << std::left
      << std::setw(W1) << attacker->name
      << std::setw(W2) << type_to_string(attacker->type)
      << std::setw(WP) << aPos
      << std::setw(WA) << action
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

FightOutcome AttackVisitor::compute(NPC& defender) {
    if (!attacker->is_alive() || !defender.is_alive())
        return FightOutcome::NobodyDied;

    int attack = std::rand() % 6 + 1;
    int defend = std::rand() % 6 + 1;

    if (attack > defend && kills(attacker->type, defender.type))
        return FightOutcome::DefenderKilled;

    if (defend > attack && kills(defender.type, attacker->type))
        return FightOutcome::AttackerKilled;

    return FightOutcome::NobodyDied;
}

FightManager& FightManager::instance() {
    static FightManager inst;
    return inst;
}

void FightManager::push(FightEvent ev) {
    std::lock_guard lock(mtx);
    queue.push(std::move(ev));
}

void FightManager::operator()()
{
    while (true) {
        std::optional<FightEvent> ev;

        {
            std::lock_guard lock(mtx);
            if (!queue.empty()) {
                ev = queue.front();
                queue.pop();
            }
        }

        if (ev) {
            auto& attacker = ev->attacker;
            auto& defender = ev->defender;

            if (!attacker || !defender) continue;
            if (!attacker->is_alive() || !defender->is_alive()) continue;

            AttackVisitor visitor(attacker);
            FightOutcome outcome = defender->accept(visitor);

            bool defender_can_be_killed = kills(attacker->type, defender->type);
            bool attacker_can_be_killed = kills(defender->type, attacker->type);

            switch (outcome) {
            case FightOutcome::DefenderKilled:
                defender->must_die();
                attacker->notify_fight(defender, FightOutcome::DefenderKilled);
                break;

            case FightOutcome::AttackerKilled:
                attacker->must_die();
                defender->notify_fight(attacker, FightOutcome::AttackerKilled);
                break;

            case FightOutcome::NobodyDied:
                if (defender_can_be_killed)
                    defender->notify_fight(attacker, FightOutcome::NobodyDied);
                if (attacker_can_be_killed)
                    attacker->notify_fight(defender, FightOutcome::NobodyDied);
                break;
            }
        }

        std::this_thread::sleep_for(50ms);
    }
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

void print_survivors(const std::vector<std::shared_ptr<NPC>>& npcs) {
    std::lock_guard<std::mutex> lck(print_mutex);
    std::cout << "\n=== Survivors ===\n";
    for (auto& npc : npcs)
        if (npc->is_alive()) {
            npc->print(std::cout);
            std::cout << '\n';
        }
}

int move_distance(NPCType type) {
    switch (type) {
        case NPCType::Orc:      return 20;
        case NPCType::Bear:     return 5;
        case NPCType::Squirrel: return 5;
        default: return 0;
    }
}

int kill_distance(NPCType type) {
    switch (type) {
        case NPCType::Orc:      return 10;
        case NPCType::Bear:     return 10;
        case NPCType::Squirrel: return 5;
        default: return 0;
    }
}

void draw_map(const std::vector<std::shared_ptr<NPC>>& list) {
    std::array<std::pair<std::string, char>, GRID * GRID> field{};
    field.fill({"", ' '});

    for (auto& npc : list) {
        if (!npc->is_alive()) continue;

        auto [x, y] = npc->position();
        int gx = std::clamp(x * GRID / MAP_X, 0, GRID - 1);
        int gy = std::clamp(y * GRID / MAP_Y, 0, GRID - 1);

        char c = '?';
        switch (npc->type) {
            case NPCType::Orc: c = 'O'; break;
            case NPCType::Bear: c = 'B'; break;
            case NPCType::Squirrel: c = 'S'; break;
            default: break;
        }

        field[gx + gy * GRID] = {npc->get_color(npc->type), c};
    }

    std::lock_guard<std::mutex> lck(print_mutex);

    for (int y = 0; y < GRID; ++y) {
        for (int x = 0; x < GRID; ++x) {
            auto [color, ch] = field[x + y * GRID];
            std::string reset = "\033[0m";  // Сброс цвета для безопасности
            std::cout << "[" << color << ch << reset << "]";
        }
        std::cout << '\n';
    }
    std::cout << std::string(3 * GRID, '=') << "\n\n";
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
