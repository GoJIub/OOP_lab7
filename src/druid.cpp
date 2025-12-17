#include "../include/druid.h"
#include "../include/npc.h"

Druid::Druid(const std::string &nm, int x_, int y_) {
    type = NPCType::Druid;
    name = nm;
    x = x_;
    y = y_;
}

InteractionOutcome Druid::accept(IInteractionVisitor &visitor) {
    return visitor.visit(*this);
}
