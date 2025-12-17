#include "../include/orc.h"
#include "../include/npc.h"

Orc::Orc(const std::string &nm, int x_, int y_) {
    type = NPCType::Orc;
    name = nm;
    x = x_;
    y = y_;
}

InteractionOutcome Orc::accept(IInteractionVisitor &visitor) {
    return visitor.visit(*this);
}
