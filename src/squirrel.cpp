#include "squirrel.h"
#include "npc.h"

Squirrel::Squirrel(const std::string &nm, int x_, int y_) {
    type = NPCType::Squirrel;
    name = nm;
    x = x_;
    y = y_;
}

FightOutcome Squirrel::accept(IFightVisitor &visitor) {
    return visitor.visit(*this);
}
