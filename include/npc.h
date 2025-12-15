#pragma once
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <shared_mutex>

struct Orc;
struct Squirrel;
struct Bear;

enum class NPCType {
    Unknown = 0,
    Orc = 1,
    Squirrel = 2,
    Bear = 3
};

enum class FightOutcome {
    AttackerKilled,
    DefenderKilled,
    NobodyDied
};

struct IFightVisitor {
    virtual FightOutcome visit(Orc &defender) = 0;
    virtual FightOutcome visit(Squirrel &defender) = 0;
    virtual FightOutcome visit(Bear &defender) = 0;
    virtual ~IFightVisitor() = default;
};

struct IFightObserver {
    virtual void on_fight(const std::shared_ptr<class NPC> &attacker,
                          const std::shared_ptr<class NPC> &defender,
                          FightOutcome outcome) = 0;
    virtual ~IFightObserver() = default;
};

struct NPC : public std::enable_shared_from_this<NPC> {
    mutable std::mutex mtx;
    NPCType type{NPCType::Unknown};
    std::string name;
    int x{0};
    int y{0};
    bool alive{true};
    std::vector<std::shared_ptr<IFightObserver>> observers;

    NPC() = default;
    NPC(NPCType t, std::string_view nm, int x_, int y_);
    virtual ~NPC() = default;

    virtual FightOutcome accept(IFightVisitor &visitor) = 0;

    void subscribe(const std::shared_ptr<IFightObserver> &obs);
    void notify_fight(const std::shared_ptr<NPC> &defender, FightOutcome outcome);

    virtual void save(std::ostream &os) const;

    virtual void print(std::ostream &os) const;

    bool is_close(const std::shared_ptr<NPC> &other, int distance) const;
    
    void move(int shift_x, int shift_y, int max_x, int max_y);

    bool is_alive() const;
    void must_die();
    std::pair<int,int> position() const;
    std::string get_color(NPCType t) const;
    int get_move_distance() const;
    int get_kill_distance() const;
};

std::string type_to_string(NPCType t);

std::shared_ptr<NPC> createNPC(NPCType type, const std::string &name, int x, int y);
std::shared_ptr<NPC> createNPCFromStream(std::istream &is);

bool kills(NPCType attacker, NPCType defender);
