#include "../include/bear.h"
#include "../include/npc.h"

Bear::Bear(const std::string &nm, int x_, int y_) {
    type = NPCType::Bear;
    name = nm;
    x = x_;
    y = y_;
}

InteractionOutcome Bear::accept(IInteractionVisitor &visitor) {
    return visitor.visit(*this);
}
