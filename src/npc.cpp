#include <cmath>
#include <sstream>
#include <stdexcept>
#include <random>
#include "npc.h"
#include "orc.h"
#include "squirrel.h"
#include "bear.h"

NPC::NPC(NPCType t, std::string_view nm, int x_, int y_)
    : type(t), name(nm), x(x_), y(y_)
{}

void NPC::subscribe(const std::shared_ptr<IFightObserver> &obs) {
    if (!obs) return;
    observers.push_back(obs);
}

void NPC::notify_fight(const std::shared_ptr<NPC>& other,
                       FightOutcome outcome)
{

    for (auto &o : observers)
        o->on_fight(std::shared_ptr<NPC>(this, [](NPC *) {}), other, outcome);
}

void NPC::save(std::ostream &os) const {
    os << static_cast<int>(type) << ' ' << name << ' ' << x << ' ' << y << '\n';
}

std::string type_to_string(NPCType t) {
    switch (t) {
        case NPCType::Orc:      return "Orc";
        case NPCType::Squirrel: return "Squirrel";
        case NPCType::Bear:     return "Bear";
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

std::pair<int,int> NPC::position() const {
    std::lock_guard<std::mutex> lck(mtx);
    return {x, y};
}

std::string NPC::get_color(NPCType t) const {
    std::lock_guard<std::mutex> lck(mtx);
    switch (t) {
        case NPCType::Orc: return "\033[31m";
        case NPCType::Bear: return "\033[33m";
        case NPCType::Squirrel: return "\033[32m";
        default: return "\033[35m";
    }
}

std::shared_ptr<NPC> createNPC(NPCType type, const std::string &name, int x, int y) {
    switch (type) {
        case NPCType::Orc: return std::make_shared<Orc>(name, x, y);
        case NPCType::Squirrel: return std::make_shared<Squirrel>(name, x, y);
        case NPCType::Bear: return std::make_shared<Bear>(name, x, y);
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

bool kills(NPCType attacker, NPCType defender) {
    // Орки убивают медведей и орков
    // Медведи убивают белок
    // Белки не хотят воевать
    if (attacker == NPCType::Orc)
        return (defender == NPCType::Bear || defender == NPCType::Orc);
    if (attacker == NPCType::Bear)
        return (defender == NPCType::Squirrel);
    return false;
}
