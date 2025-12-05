#include "bear.h"
#include "npc.h"

Bear::Bear(const std::string &nm, int x_, int y_) {
    type = NPCType::Bear;
    name = nm;
    x = x_;
    y = y_;
}

FightOutcome Bear::accept(IFightVisitor &visitor) {
    return visitor.visit(*this);
}
