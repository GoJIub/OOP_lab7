#include "../include/squirrel.h"
#include "../include/npc.h"

Squirrel::Squirrel(const std::string &nm, int x_, int y_) {
    type = NPCType::Squirrel;
    name = nm;
    x = x_;
    y = y_;
}

InteractionOutcome Squirrel::accept(IInteractionVisitor &visitor) {
    return visitor.visit(*this);
}
