#include "../include/game_utils.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unordered_set>
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

    case FightOutcome::DefenderEscaped:
        std::cout << ">>> "
                  << defender->name << " (" << type_to_string(defender->type) << ")"
                  << " escaped from "
                  << attacker->name << " (" << type_to_string(attacker->type) << ")\n";
        break;

    case FightOutcome::NoFight:
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

    switch (outcome) {
    case FightOutcome::DefenderKilled:
        f << std::left
          << std::setw(W1) << attacker->name
          << std::setw(W2) << type_to_string(attacker->type)
          << std::setw(WP) << aPos
          << std::setw(WA) << "killed"
          << std::setw(W3) << defender->name
          << std::setw(W4) << type_to_string(defender->type)
          << std::setw(WP2) << dPos
          << "\n";
        break;

    case FightOutcome::DefenderEscaped:
        f << std::left
          << std::setw(W1) << defender->name
          << std::setw(W2) << type_to_string(defender->type)
          << std::setw(WP) << dPos
          << std::setw(WA) << "escaped"
          << std::setw(W3) << attacker->name
          << std::setw(W4) << type_to_string(attacker->type)
          << std::setw(WP2) << aPos
          << "\n";
        break;

    case FightOutcome::NoFight:
        break;
    }
}

// ---------------- Логика боя ----------------
AttackVisitor::AttackVisitor(const std::shared_ptr<NPC>& attacker_)
    : attacker(attacker_) {}

FightOutcome AttackVisitor::visit([[maybe_unused]] Orc& defender) {
    if (!attacker->is_alive()) return FightOutcome::NoFight;

    if (attacker->type == NPCType::Orc)
        return dice() ? FightOutcome::DefenderKilled : FightOutcome::DefenderEscaped;

    return FightOutcome::NoFight;
}

FightOutcome AttackVisitor::visit([[maybe_unused]] Bear& defender) {
    if (!attacker->is_alive()) return FightOutcome::NoFight;

    if (attacker->type == NPCType::Orc)
        return dice() ? FightOutcome::DefenderKilled : FightOutcome::DefenderEscaped;

    return FightOutcome::NoFight;
}

FightOutcome AttackVisitor::visit([[maybe_unused]] Squirrel& defender) {
    if (!attacker->is_alive()) return FightOutcome::NoFight;

    if (attacker->type == NPCType::Bear)
        return dice() ? FightOutcome::DefenderKilled : FightOutcome::DefenderEscaped;

    return FightOutcome::NoFight;
}

bool AttackVisitor::dice() {
    return roll() > roll();
}

FightManager& FightManager::instance() {
    static FightManager inst;
    return inst;
}

void FightManager::push(FightEvent ev) {
    std::lock_guard<std::mutex> lock(mtx);
    queue.push(std::move(ev));
}

void FightManager::operator()() {
    using namespace std::chrono_literals;

    while (running) {
        std::optional<FightEvent> ev;

        {
            std::lock_guard<std::mutex> lock(mtx);
            if (!queue.empty()) {
                ev = queue.front();
                queue.pop();
            }
        }

        if (ev) {
            auto attacker = ev->attacker;
            auto defender = ev->defender;

            if (!attacker || !defender || !attacker->is_alive() || !defender->is_alive()) {
                std::this_thread::sleep_for(100ms);
                continue;
            }

            if (!attacker->is_close(defender, attacker->get_kill_distance()))
                continue;

            AttackVisitor visitor1(attacker);
            FightOutcome outcome1 = defender->accept(visitor1);

            AttackVisitor visitor2(defender);
            FightOutcome outcome2 = attacker->accept(visitor2);

            switch (outcome1) {
            case FightOutcome::DefenderKilled:
                defender->must_die();
                attacker->notify_fight(defender, FightOutcome::DefenderKilled);
                break;

            case FightOutcome::DefenderEscaped:
                attacker->notify_fight(defender, FightOutcome::DefenderEscaped);
                break;

            case FightOutcome::NoFight:
                defender->notify_fight(attacker, FightOutcome::NoFight);
                break;
            }

            switch (outcome2) {
            case FightOutcome::DefenderKilled:
                attacker->must_die();
                defender->notify_fight(attacker, FightOutcome::DefenderKilled);
                break;

            case FightOutcome::DefenderEscaped:
                if (defender->is_alive())
                    defender->notify_fight(attacker, FightOutcome::DefenderEscaped);
                break;

            case FightOutcome::NoFight:
                attacker->notify_fight(defender, FightOutcome::NoFight);
                break;
            }
        }
        std::this_thread::sleep_for(10ms);
    }
}

void FightManager::stop() {
    running = false;
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

void draw_map(const std::vector<std::shared_ptr<NPC>>& list) {
    std::array<std::pair<std::string, char>, GRID * GRID> field{};
    field.fill({"", ' '});

    for (auto& npc : list) {
        int x, y;
        if (!npc->get_state(x, y)) continue;
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

    std::cout << std::string(3 * GRID, '=') << "\n";
    for (int y = 0; y < GRID; ++y) {
        for (int x = 0; x < GRID; ++x) {
            auto [color, ch] = field[x + y * GRID];
            std::string reset = "\033[0m";
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

std::mt19937& rng() {
    static std::mt19937 gen{std::random_device{}()};
    return gen;
}

int roll() {
    static std::uniform_int_distribution<int> d(1, 6);
    return d(rng());
}
