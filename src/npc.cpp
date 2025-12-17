#include <cmath>
#include <sstream>
#include <stdexcept>
#include <random>
#include "../include/npc.h"
#include "../include/orc.h"
#include "../include/squirrel.h"
#include "../include/bear.h"
#include "../include/druid.h"

NPC::NPC(NPCType t, std::string_view nm, int x_, int y_)
    : type(t), name(nm), x(x_), y(y_)
{}

void NPC::subscribe(const std::shared_ptr<IInteractionObserver> &obs) {
    if (!obs) return;
    observers.push_back(obs);
}

void NPC::notify_interaction(const std::shared_ptr<NPC>& other,
                       InteractionOutcome outcome)
{

    for (auto &o : observers)
        o->on_interaction(std::shared_ptr<NPC>(this, [](NPC *) {}), other, outcome);
}

void NPC::save(std::ostream &os) const {
    os << static_cast<int>(type) << ' ' << name << ' ' << x << ' ' << y << '\n';
}

std::string type_to_string(NPCType t) {
    switch (t) {
        case NPCType::Orc:      return "Orc";
        case NPCType::Squirrel: return "Squirrel";
        case NPCType::Bear:     return "Bear";
        case NPCType::Druid:    return "Druid";
        default:                return "Unknown";
    }
}

void NPC::print(std::ostream &os) const {
    os << name << " [" << type_to_string(type) << "] at (" << x << "," << y << ")";
}

bool NPC::is_close(const std::shared_ptr<NPC> &other, int distance) const {
    if (std::pow(x - other->x, 2) + std::pow(y - other->y, 2) <= std::pow(distance, 2))
        return true;
    else
        return false;
}

void NPC::move(int shift_x, int shift_y, int max_x, int max_y) {
    std::lock_guard<std::mutex> lck(mtx);

    if ((x + shift_x >= 0) && (x + shift_x <= max_x))
        x += shift_x;
    if ((y + shift_y >= 0) && (y + shift_y <= max_y))
        y += shift_y;
}

bool NPC::is_alive() const {
    std::lock_guard<std::mutex> lck(mtx);
    return alive;
}

void NPC::must_die() {
    std::lock_guard<std::mutex> lck(mtx);
    alive = false;
}

void NPC::heal() {
    std::lock_guard<std::mutex> lck(mtx);
    alive = true;
}

std::pair<int,int> NPC::position() const {
    std::lock_guard<std::mutex> lck(mtx);
    return {x, y};
}

std::string NPC::get_color(NPCType t) const {
    std::lock_guard<std::mutex> lck(mtx);
    switch (t) {
        case NPCType::Orc:      return "\033[31m";
        case NPCType::Bear:     return "\033[33m";
        case NPCType::Squirrel: return "\033[32m";
        case NPCType::Druid:    return "\033[36m";
        default:                return "\033[35m";
    }
}

int NPC::get_move_distance() const {
    switch (type) {
        case NPCType::Orc:      return 20;
        case NPCType::Bear:     return 5;
        case NPCType::Squirrel: return 5;
        case NPCType::Druid:    return 10;
        default:                return 0;
    }
}

int NPC::get_interaction_distance() const {
    switch (type) {
        case NPCType::Orc:      return 10;
        case NPCType::Bear:     return 10;
        case NPCType::Squirrel: return 5;
        case NPCType::Druid:    return 10;
        default:                return 0;
    }
}

bool NPC::get_state(int& x_, int& y_) const {
    std::lock_guard<std::mutex> lck(mtx);
    if (!alive) return false;
    x_ = x;
    y_ = y;
    return true;
}

std::shared_ptr<NPC> createNPC(NPCType type, const std::string &name, int x, int y) {
    switch (type) {
        case NPCType::Orc:      return std::make_shared<Orc>(name, x, y);
        case NPCType::Squirrel: return std::make_shared<Squirrel>(name, x, y);
        case NPCType::Bear:     return std::make_shared<Bear>(name, x, y);
        case NPCType::Druid:    return std::make_shared<Druid>(name, x, y);
        default: return nullptr;
    }
}

std::shared_ptr<NPC> createNPCFromStream(std::istream &is) {
    int t;
    std::string name;
    int x, y;
    if (!(is >> t >> name >> x >> y)) return nullptr;
    return createNPC(static_cast<NPCType>(t), name, x, y);
}
