#pragma once
#include <vector>
#include <memory>
#include <string>
#include "npc.h"
#include "orc.h"
#include "squirrel.h"
#include "bear.h"

// ---------------- Наблюдатели ----------------
struct ConsoleObserver : public IFightObserver {
    void on_fight(const std::shared_ptr<NPC> &attacker,
                  const std::shared_ptr<NPC> &defender,
                  bool win) override;
};

struct FileObserver : public IFightObserver {
    FileObserver(const std::string &filename);
    void on_fight(const std::shared_ptr<NPC> &attacker,
                  const std::shared_ptr<NPC> &defender,
                  bool win) override;
private:
    std::string fname;
    static const int W1, W2, WP, WA, W3, W4, WP2;
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

// ---------------- Вспомогательные функции ----------------
void save_all(const std::vector<std::shared_ptr<NPC>> &list, const std::string &filename);
std::vector<std::shared_ptr<NPC>> load_all(const std::string &filename);
void print_all(const std::vector<std::shared_ptr<NPC>> &list);
std::vector<std::shared_ptr<NPC>> fight_round(std::vector<std::shared_ptr<NPC>> &list, int distance);
NPCType random_type();
int random_coord(int min, int max);
