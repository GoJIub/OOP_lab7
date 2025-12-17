#pragma once
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <shared_mutex>

struct Orc;
struct Squirrel;
struct Bear;
struct Druid;

enum class NPCType {
    Unknown = 0,
    Orc = 1,
    Squirrel = 2,
    Bear = 3,
    Druid = 4
};

enum class InteractionOutcome {
    TargetKilled,
    TargetEscaped,
    TargetHealed,
    NoInteraction
};

struct IInteractionVisitor {
    virtual InteractionOutcome visit(Orc &target) = 0;
    virtual InteractionOutcome visit(Squirrel &target) = 0;
    virtual InteractionOutcome visit(Bear &target) = 0;
    virtual InteractionOutcome visit(Druid &target) = 0;
    virtual ~IInteractionVisitor() = default;
};

struct IInteractionObserver {
    virtual void on_interaction(const std::shared_ptr<class NPC> &actor,
                          const std::shared_ptr<class NPC> &target,
                          InteractionOutcome outcome) = 0;
    virtual ~IInteractionObserver() = default;
};

struct NPC : public std::enable_shared_from_this<NPC> {
    mutable std::mutex mtx;
    NPCType type{NPCType::Unknown};
    std::string name;
    int x{0};
    int y{0};
    bool alive{true};
    std::vector<std::shared_ptr<IInteractionObserver>> observers;

    NPC() = default;
    NPC(NPCType t, std::string_view nm, int x_, int y_);
    virtual ~NPC() = default;

    virtual InteractionOutcome accept(IInteractionVisitor &visitor) = 0;

    void subscribe(const std::shared_ptr<IInteractionObserver> &obs);
    void notify_interaction(const std::shared_ptr<NPC> &target, InteractionOutcome outcome);

    virtual void save(std::ostream &os) const;

    virtual void print(std::ostream &os) const;

    bool is_close(const std::shared_ptr<NPC> &other, int distance) const;
    
    void move(int shift_x, int shift_y, int max_x, int max_y);

    bool is_alive() const;
    void must_die();
    void heal();
    std::pair<int,int> position() const;
    std::string get_color(NPCType t) const;
    int get_move_distance() const;
    int get_interaction_distance() const;
    bool get_state(int& x_, int& y_) const;
};

std::string type_to_string(NPCType t);

std::shared_ptr<NPC> createNPC(NPCType type, const std::string &name, int x, int y);
std::shared_ptr<NPC> createNPCFromStream(std::istream &is);
