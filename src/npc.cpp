#include <cmath>
#include <sstream>
#include <stdexcept>
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

void NPC::notify_kill(const std::shared_ptr<NPC> &victim) {
    for (auto &w : observers)
        if (auto s = w.lock())
            s->on_fight(shared_from_this(), victim, true);
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
