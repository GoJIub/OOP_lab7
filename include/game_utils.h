#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <string>
#include <atomic>
#include <condition_variable>
#include "npc.h"
#include "orc.h"
#include "squirrel.h"
#include "bear.h"

constexpr int MAP_X = 100;
constexpr int MAP_Y = 100;
constexpr int GRID = 20;

// ---------------- Наблюдатели ----------------
class ConsoleObserver : public IFightObserver {
private:
    ConsoleObserver() {};

public:
    static std::shared_ptr<IFightObserver> get();
    void on_fight(const std::shared_ptr<NPC> &attacker,
                  const std::shared_ptr<NPC> &defender,
                  FightOutcome outcome) override;
};

class FileObserver : public IFightObserver {
private:
    explicit FileObserver(const std::string& filename);
    std::string fname;
    static const int W1, W2, WP, WA, W3, W4, WP2;

public:
    static std::shared_ptr<IFightObserver> get(const std::string& filename);
    void on_fight(const std::shared_ptr<NPC> &attacker,
                  const std::shared_ptr<NPC> &defender,
                  FightOutcome outcome) override;
};

// ---------------- Логика боя ----------------
struct AttackVisitor : public IFightVisitor {
    explicit AttackVisitor(const std::shared_ptr<NPC> &attacker_);
    FightOutcome visit(Orc &defender) override;
    FightOutcome visit(Squirrel &defender) override;
    FightOutcome visit(Bear &defender) override;
private:
    std::shared_ptr<NPC> attacker;
    FightOutcome compute(NPC &defender);
};

struct FightEvent {
    std::shared_ptr<NPC> attacker;
    std::shared_ptr<NPC> defender;
};

class FightManager {
public:
    static FightManager& instance();

    void push(FightEvent ev);
    void operator()();
    void stop();

private:
    FightManager() = default;
    std::queue<FightEvent> queue;
    std::mutex mtx;
    std::atomic<bool> running{true};
};

// ---------------- Вспомогательные функции ----------------
void save_all(const std::vector<std::shared_ptr<NPC>> &list, const std::string &filename);
std::vector<std::shared_ptr<NPC>> load_all(const std::string &filename);
void print_all(const std::vector<std::shared_ptr<NPC>> &list);
void print_survivors(const std::vector<std::shared_ptr<NPC>>& npcs);
int move_distance(NPCType type);
int kill_distance(NPCType type);
void draw_map(const std::vector<std::shared_ptr<NPC>>& list);
NPCType random_type();
int random_coord(int min, int max);
