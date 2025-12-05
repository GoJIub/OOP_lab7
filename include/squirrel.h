#pragma once
#include "npc.h"

struct Squirrel : public NPC {
    Squirrel() = default;
    Squirrel(const std::string &nm, int x_, int y_);
    FightOutcome accept(IFightVisitor &visitor) override;
};
