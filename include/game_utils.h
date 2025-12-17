#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <string>
#include <atomic>
#include <random>
#include "npc.h"
#include "orc.h"
#include "squirrel.h"
#include "bear.h"

constexpr int MAP_X = 100;
constexpr int MAP_Y = 100;
constexpr int GRID = 20;

// ---------------- Наблюдатели ----------------
class ConsoleObserver : public IInteractionObserver {
private:
    ConsoleObserver() {};

public:
    static std::shared_ptr<IInteractionObserver> get();
    void on_interaction(const std::shared_ptr<NPC> &actor,
                  const std::shared_ptr<NPC> &target,
                  InteractionOutcome outcome) override;
};

class FileObserver : public IInteractionObserver {
private:
    explicit FileObserver(const std::string& filename);
    std::string fname;
    static const int W1, W2, WP, WA, W3, W4, WP2;

public:
    static std::shared_ptr<IInteractionObserver> get(const std::string& filename);
    void on_interaction(const std::shared_ptr<NPC> &actor,
                  const std::shared_ptr<NPC> &target,
                  InteractionOutcome outcome) override;
};

// ---------------- Логика боя ----------------
struct AttackVisitor : public IInteractionVisitor {
    explicit AttackVisitor(const std::shared_ptr<NPC> &actor_);
    InteractionOutcome visit([[maybe_unused]] Orc& target) override;
    InteractionOutcome visit([[maybe_unused]] Bear& target) override;
    InteractionOutcome visit([[maybe_unused]] Squirrel& target) override;
    InteractionOutcome visit([[maybe_unused]] Druid& target) override;
private:
    std::shared_ptr<NPC> actor;
    bool dice();
};

struct SupportVisitor : public IInteractionVisitor {
    explicit SupportVisitor(const std::shared_ptr<NPC>& actor_);

    InteractionOutcome visit(Orc&) override;
    InteractionOutcome visit(Bear&) override;
    InteractionOutcome visit(Squirrel&) override;
    InteractionOutcome visit(Druid&) override;

private:
    std::shared_ptr<NPC> actor;
};

struct InteractionEvent {
    std::shared_ptr<NPC> actor;
    std::shared_ptr<NPC> target;
};

class InteractionManager {
public:
    static InteractionManager& instance();

    void push(InteractionEvent ev);
    void apply_outcome(const std::shared_ptr<NPC>& actor,
                   const std::shared_ptr<NPC>& target,
                   InteractionOutcome outcome);
    void operator()();
    void stop();

private:
    InteractionManager() = default;
    std::queue<InteractionEvent> queue;
    std::mutex mtx;
    std::atomic<bool> running{true};
};

// ---------------- Вспомогательные функции ----------------
void save_all(const std::vector<std::shared_ptr<NPC>> &list, const std::string &filename);
std::vector<std::shared_ptr<NPC>> load_all(const std::string &filename);
void print_all(const std::vector<std::shared_ptr<NPC>> &list);
void print_survivors(const std::vector<std::shared_ptr<NPC>>& npcs);
void draw_map(const std::vector<std::shared_ptr<NPC>>& list);
NPCType random_type();
int random_coord(int min, int max);
std::mt19937& rng();
int roll();
