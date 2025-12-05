#include "orc.h"
#include "npc.h"

Orc::Orc(const std::string &nm, int x_, int y_) {
    type = NPCType::Orc;
    name = nm;
    x = x_;
    y = y_;
}

FightOutcome Orc::accept(IFightVisitor &visitor) {
    return visitor.visit(*this);
}
