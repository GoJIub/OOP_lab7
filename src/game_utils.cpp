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
std::shared_ptr<IInteractionObserver> ConsoleObserver::get() {
    static ConsoleObserver instance;
    return std::shared_ptr<IInteractionObserver>(&instance, [](IInteractionObserver*) {});
}

void ConsoleObserver::on_interaction(const std::shared_ptr<NPC>& actor,
                               const std::shared_ptr<NPC>& target,
                               InteractionOutcome outcome)
{
    if (!actor || !target) return;

    std::lock_guard<std::mutex> lck(print_mutex);

    switch (outcome) {
    case InteractionOutcome::TargetKilled:
        std::cout << ">>> "
                  << actor->name << " (" << type_to_string(actor->type) << ")"
                  << " killed "
                  << target->name << " (" << type_to_string(target->type) << ")\n";
        break;

    case InteractionOutcome::TargetEscaped:
        std::cout << ">>> "
                  << target->name << " (" << type_to_string(target->type) << ")"
                  << " escaped from "
                  << actor->name << " (" << type_to_string(actor->type) << ")\n";
        break;

    case InteractionOutcome::TargetHealed:
        std::cout << ">>> "
                << actor->name << " (Druid)"
                << "healed"
                << target->name << " (" << type_to_string(target->type) << ")\n";
        break;

    case InteractionOutcome::NoInteraction:
        break;
    }
}

FileObserver::FileObserver(const std::string& filename) : fname(filename)
{
    std::ofstream f(fname, std::ios::trunc);
    if (!f.good()) return;

    std::lock_guard<std::mutex> lck(print_mutex);

    f << std::left
      << std::setw(W1)  << "Actor"
      << std::setw(W2)  << "Type"
      << std::setw(WP)  << "Pos"
      << std::setw(WA)  << "Action"
      << std::setw(W3)  << "Target"
      << std::setw(W4)  << "Type"
      << std::setw(WP2) << "Pos"
      << "\n";

    f << std::string(W1 + W2 + WP + WA + W3 + W4 + WP2, '-') << "\n";
}

std::shared_ptr<IInteractionObserver> FileObserver::get(const std::string& filename)
{
    static std::shared_ptr<IInteractionObserver> instance =
        std::shared_ptr<IInteractionObserver>(
            new FileObserver(filename),
            [](IInteractionObserver*){}
        );

    return instance;
}

void FileObserver::on_interaction(const std::shared_ptr<NPC>& actor,
                            const std::shared_ptr<NPC>& target,
                            InteractionOutcome outcome)
{
    if (!actor || !target) return;

    std::ofstream f(fname, std::ios::app);
    if (!f.good()) return;

    std::lock_guard<std::mutex> lck(print_mutex);

    std::ostringstream ss;
    ss << '(' << actor->x << ',' << actor->y << ')';
    std::string aPos = ss.str();
    ss.str("");
    ss << '(' << target->x << ',' << target->y << ')';
    std::string tPos = ss.str();

    switch (outcome) {
    case InteractionOutcome::TargetKilled:
        f << std::left
          << std::setw(W1) << actor->name
          << std::setw(W2) << type_to_string(actor->type)
          << std::setw(WP) << aPos
          << std::setw(WA) << "killed"
          << std::setw(W3) << target->name
          << std::setw(W4) << type_to_string(target->type)
          << std::setw(WP2) << tPos
          << "\n";
        break;

    case InteractionOutcome::TargetEscaped:
        f << std::left
          << std::setw(W1) << target->name
          << std::setw(W2) << type_to_string(target->type)
          << std::setw(WP) << tPos
          << std::setw(WA) << "escaped"
          << std::setw(W3) << actor->name
          << std::setw(W4) << type_to_string(actor->type)
          << std::setw(WP2) << aPos
          << "\n";
        break;

    case InteractionOutcome::TargetHealed:
        f << std::left
          << std::setw(W1) << actor->name
          << std::setw(W2) << type_to_string(actor->type)
          << std::setw(WP) << aPos
          << std::setw(WA) << "healed"
          << std::setw(W3) << target->name
          << std::setw(W4) << type_to_string(target->type)
          << std::setw(WP2) << tPos
          << "\n";
        break;

    case InteractionOutcome::NoInteraction:
        break;
    }
}

// ---------------- Логика боя ----------------
AttackVisitor::AttackVisitor(const std::shared_ptr<NPC>& actor_)
    : actor(actor_) {}

InteractionOutcome AttackVisitor::visit([[maybe_unused]] Orc& target) {
    if (!actor->is_alive()) return InteractionOutcome::NoInteraction;

    if (actor->type == NPCType::Orc)
        return dice() ? InteractionOutcome::TargetKilled : InteractionOutcome::TargetEscaped;

    return InteractionOutcome::NoInteraction;
}

InteractionOutcome AttackVisitor::visit([[maybe_unused]] Bear& target) {
    if (!actor->is_alive()) return InteractionOutcome::NoInteraction;

    if (actor->type == NPCType::Orc)
        return dice() ? InteractionOutcome::TargetKilled : InteractionOutcome::TargetEscaped;

    return InteractionOutcome::NoInteraction;
}

InteractionOutcome AttackVisitor::visit([[maybe_unused]] Squirrel& target) {
    if (!actor->is_alive()) return InteractionOutcome::NoInteraction;

    if (actor->type == NPCType::Bear)
        return dice() ? InteractionOutcome::TargetKilled : InteractionOutcome::TargetEscaped;

    return InteractionOutcome::NoInteraction;
}

InteractionOutcome AttackVisitor::visit([[maybe_unused]] Druid& target) {
    if (!actor->is_alive()) return InteractionOutcome::NoInteraction;

    if (actor->type == NPCType::Orc)
        return dice() ? InteractionOutcome::TargetKilled : InteractionOutcome::TargetEscaped;

    return InteractionOutcome::NoInteraction;
}

bool AttackVisitor::dice() {
    return roll() > roll();
}

SupportVisitor::SupportVisitor(const std::shared_ptr<NPC>& actor_)
    : actor(actor_) {}

InteractionOutcome SupportVisitor::visit(Orc&) {
    return InteractionOutcome::NoInteraction;
}

InteractionOutcome SupportVisitor::visit(Bear& target) {
    if (actor->type == NPCType::Druid && !target.is_alive())
        return InteractionOutcome::TargetHealed;

    return InteractionOutcome::NoInteraction;
}

InteractionOutcome SupportVisitor::visit(Squirrel& target) {
    if (actor->type == NPCType::Druid && !target.is_alive())
        return InteractionOutcome::TargetHealed;

    return InteractionOutcome::NoInteraction;
}

InteractionOutcome SupportVisitor::visit(Druid&) {
    return InteractionOutcome::NoInteraction;
}

InteractionManager& InteractionManager::instance() {
    static InteractionManager inst;
    return inst;
}

void InteractionManager::push(InteractionEvent ev) {
    std::lock_guard<std::mutex> lock(mtx);
    queue.push(std::move(ev));
}

void InteractionManager::apply_outcome(const std::shared_ptr<NPC>& actor,
                   const std::shared_ptr<NPC>& target,
                   InteractionOutcome outcome)
{
    switch (outcome) {
    case InteractionOutcome::TargetKilled:
        target->must_die();
        actor->notify_interaction(target, outcome);
        break;

    case InteractionOutcome::TargetEscaped:
        actor->notify_interaction(target, outcome);
        break;

    case InteractionOutcome::TargetHealed:
        target->heal();
        actor->notify_interaction(target, outcome);
        break;

    case InteractionOutcome::NoInteraction:
        break;
    }
}

void InteractionManager::operator()() {
    using namespace std::chrono_literals;

    while (running) {
        std::optional<InteractionEvent> ev;

        {
            std::lock_guard<std::mutex> lock(mtx);
            if (!queue.empty()) {
                ev = queue.front();
                queue.pop();
            }
        }

        if (ev) {
            auto a = ev->actor;
            auto t = ev->target;

            if (!a || !t) {
                std::this_thread::sleep_for(100ms);
                continue;
            }

            if (a->is_alive() && t->is_alive() &&
                a->is_close(t, a->get_interaction_distance()))
            {
                AttackVisitor av1(a);
                apply_outcome(a, t, t->accept(av1));
                AttackVisitor av2(t);
                apply_outcome(t, a, a->accept(av2));
            }

            if ((a->is_alive() || t->is_alive()) &&
                a->is_close(t, a->get_interaction_distance())) {
                SupportVisitor sv1(a);
                apply_outcome(a, t, t->accept(sv1));

                SupportVisitor sv2(t);
                apply_outcome(t, a, a->accept(sv2));
            }

            std::this_thread::sleep_for(10ms);
        }
        std::this_thread::sleep_for(10ms);
    }
}

void InteractionManager::stop() {
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
        auto [x, y] = npc->position();

        int gx = std::clamp(x * GRID / MAP_X, 0, GRID - 1);
        int gy = std::clamp(y * GRID / MAP_Y, 0, GRID - 1);

        char c;
        if (!npc->is_alive())
            c = '*';
        else {
            switch (npc->type) {
                case NPCType::Orc: c = 'O'; break;
                case NPCType::Bear: c = 'B'; break;
                case NPCType::Squirrel: c = 'S'; break;
                case NPCType::Druid: c = 'D'; break;
                default: c = '?';
            }
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
    std::uniform_int_distribution<int> dist(1, 4);
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
