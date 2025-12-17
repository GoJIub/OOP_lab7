#pragma once
#include "npc.h"

struct Druid : public NPC {
    Druid() = default;
    Druid(const std::string &nm, int x_, int y_);
    InteractionOutcome accept(IInteractionVisitor &visitor) override;
};
