#pragma once
#include "npc.h"

struct Bear : public NPC {
    Bear() = default;
    Bear(const std::string &nm, int x_, int y_);
    FightOutcome accept(IFightVisitor &visitor) override;
};
