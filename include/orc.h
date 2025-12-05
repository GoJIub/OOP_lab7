#pragma once
#include "npc.h"

struct Orc : public NPC {
    Orc() = default;
    Orc(const std::string &nm, int x_, int y_);
    FightOutcome accept(IFightVisitor &visitor) override;
};
