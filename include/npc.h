#pragma once
#include <memory>
#include <string>
#include <vector>
#include <iostream>

struct Orc;
struct Squirrel;
struct Bear;

enum class NPCType {
    Unknown = 0,
    Orc = 1,
    Squirrel = 2,
    Bear = 3
};

struct FightOutcome {
    bool attackerDead = false;
    bool defenderDead = false;
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
                          bool win) = 0;
    virtual ~IFightObserver() = default;
};

struct NPC : public std::enable_shared_from_this<NPC> {
    NPCType type{NPCType::Unknown};
    std::string name;
    int x{0};
    int y{0};
    std::vector<std::weak_ptr<IFightObserver>> observers;

    NPC() = default;
    NPC(NPCType t, std::string_view nm, int x_, int y_);
    virtual ~NPC() = default;

    virtual FightOutcome accept(IFightVisitor &visitor) = 0;

    void subscribe(const std::shared_ptr<IFightObserver> &obs);
    void notify_kill(const std::shared_ptr<NPC> &victim);

    virtual void save(std::ostream &os) const;

    virtual void print(std::ostream &os) const;

    bool is_close(const std::shared_ptr<NPC> &other, int distance) const;
};

std::string type_to_string(NPCType t);

std::shared_ptr<NPC> createNPC(NPCType type, const std::string &name, int x, int y);
std::shared_ptr<NPC> createNPCFromStream(std::istream &is);

bool kills(NPCType attacker, NPCType defender);
