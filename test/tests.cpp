#include <gtest/gtest.h>
#include <memory>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include "../include/npc.h"
#include "../include/orc.h"
#include "../include/squirrel.h"
#include "../include/bear.h"
#include "../include/game_utils.h"

// ---------------- HELPERS ----------------
struct TestObserver : public IFightObserver {
    std::vector<std::string> logs;
    void on_fight(const std::shared_ptr<NPC>& attacker,
                  const std::shared_ptr<NPC>& defender,
                  bool win) override {
        if (win)
            logs.push_back(attacker->name + "->" + defender->name);
    }
};

// ---------------- NPC Creation ----------------
TEST(NPCTest, CreateOrc) {
    auto o = createNPC(NPCType::Orc, "Orc1", 10, 20);
    ASSERT_TRUE(o);
    EXPECT_EQ(o->type, NPCType::Orc);
    EXPECT_EQ(o->name, "Orc1");
    EXPECT_EQ(o->x, 10);
    EXPECT_EQ(o->y, 20);
}

TEST(NPCTest, CreateSquirrel) {
    auto s = createNPC(NPCType::Squirrel, "S1", 0, 0);
    ASSERT_TRUE(s);
    EXPECT_EQ(s->type, NPCType::Squirrel);
    EXPECT_EQ(s->name, "S1");
}

TEST(NPCTest, CreateBear) {
    auto b = createNPC(NPCType::Bear, "B1", 5, 5);
    ASSERT_TRUE(b);
    EXPECT_EQ(b->type, NPCType::Bear);
}

// ---------------- NPC::is_close ----------------
TEST(NPCTest, IsCloseTrue) {
    auto a = createNPC(NPCType::Orc, "O1", 0, 0);
    auto b = createNPC(NPCType::Bear, "B1", 3, 4); // distance = 5
    EXPECT_TRUE(a->is_close(b, 5));
    EXPECT_FALSE(a->is_close(b, 4));
}

// ---------------- kills() ----------------
TEST(KillsTest, OrcKillsBearOrOrc) {
    EXPECT_TRUE(kills(NPCType::Orc, NPCType::Bear));
    EXPECT_TRUE(kills(NPCType::Orc, NPCType::Orc));
    EXPECT_FALSE(kills(NPCType::Orc, NPCType::Squirrel));
}

TEST(KillsTest, BearKillsSquirrel) {
    EXPECT_TRUE(kills(NPCType::Bear, NPCType::Squirrel));
    EXPECT_FALSE(kills(NPCType::Bear, NPCType::Orc));
}

TEST(KillsTest, SquirrelDoesNotKill) {
    EXPECT_FALSE(kills(NPCType::Squirrel, NPCType::Orc));
    EXPECT_FALSE(kills(NPCType::Squirrel, NPCType::Bear));
    EXPECT_FALSE(kills(NPCType::Squirrel, NPCType::Squirrel));
}

// ---------------- Observer ----------------
TEST(ObserverTest, NotifyKill) {
    auto o = createNPC(NPCType::Orc, "O1", 0, 0);
    auto b = createNPC(NPCType::Bear, "B1", 0, 0);
    auto obs = std::make_shared<TestObserver>();

    o->subscribe(obs);
    o->notify_kill(b);

    ASSERT_EQ(obs->logs.size(), 1u);
    EXPECT_EQ(obs->logs[0], "O1->B1");
}

// ---------------- FightVisitor ----------------
struct DummyVisitor : public IFightVisitor {
    FightOutcome visit(Orc&) override { return {false,false}; }
    FightOutcome visit(Squirrel&) override { return {false,false}; }
    FightOutcome visit(Bear&) override { return {false,true}; }
};

TEST(VisitorTest, VisitOrc) {
    auto o = createNPC(NPCType::Orc, "O1",0,0);
    DummyVisitor v;
    auto res = o->accept(v);
    EXPECT_FALSE(res.attackerDead);
    EXPECT_FALSE(res.defenderDead);
}

TEST(VisitorTest, VisitSquirrel) {
    auto s = createNPC(NPCType::Squirrel, "S1",0,0);
    DummyVisitor v;
    auto res = s->accept(v);
    EXPECT_FALSE(res.defenderDead);
}

// ---------------- Save/Load ----------------
TEST(SaveLoadTest, SaveLoadNPC) {
    std::vector<std::shared_ptr<NPC>> list;
    list.push_back(createNPC(NPCType::Orc, "O1", 1, 2));
    list.push_back(createNPC(NPCType::Squirrel, "S1", 3, 4));

    std::string fname = "tmp_npcs.txt";
    save_all(list, fname);

    auto loaded = load_all(fname);
    EXPECT_EQ(loaded.size(), 2u);
    EXPECT_EQ(loaded[0]->name, "O1");
    EXPECT_EQ(loaded[1]->x, 3);

    std::remove(fname.c_str());
}

// ---------------- print_all (smoke test) ----------------
TEST(PrintAllTest, PrintDoesNotCrash) {
    auto o = createNPC(NPCType::Orc, "O1", 1, 1);
    auto s = createNPC(NPCType::Squirrel, "S1", 2, 2);
    std::vector<std::shared_ptr<NPC>> list{o,s};

    EXPECT_NO_THROW(print_all(list));
}

// ---------------- fight_round ----------------
TEST(FightRoundTest, OneKillsOther) {
    auto o = createNPC(NPCType::Orc,"O1",0,0);
    auto b = createNPC(NPCType::Bear,"B1",0,0);
    std::vector<std::shared_ptr<NPC>> list{o,b};

    auto dead = fight_round(list, 1);
    ASSERT_EQ(dead.size(), 1u);
    EXPECT_EQ(dead[0]->name,"B1");
}

// ---------------- fight_round multiple ----------------
TEST(FightRoundTest, MultipleKills) {
    std::vector<std::shared_ptr<NPC>> list;
    for(int i=0;i<3;i++)
        list.push_back(createNPC(NPCType::Orc,"O"+std::to_string(i),0,0));
    list.push_back(createNPC(NPCType::Bear,"B1",0,0));
    auto dead = fight_round(list,1);
    std::vector<std::string> dead_names;
    for(auto &d : dead) dead_names.push_back(d->name);

    EXPECT_EQ(dead.size(), 3u);
    EXPECT_TRUE(std::find(dead_names.begin(), dead_names.end(), "B1") != dead_names.end());
}

// ---------------- Observer called in fight_round ----------------
TEST(FightRoundObserverTest, ObserverCalled) {
    auto o = createNPC(NPCType::Orc,"O1",0,0);
    auto b = createNPC(NPCType::Bear,"B1",0,0);
    auto obs = std::make_shared<TestObserver>();
    o->subscribe(obs);

    std::vector<std::shared_ptr<NPC>> list{o,b};
    fight_round(list,1);

    EXPECT_EQ(obs->logs.size(), 1u);
    EXPECT_EQ(obs->logs[0],"O1->B1");
}

// ---------------- is_close edge ----------------
TEST(NPCTest, IsCloseExactDistance) {
    auto o = createNPC(NPCType::Orc,"O1",0,0);
    auto b = createNPC(NPCType::Bear,"B1",3,4); // distance=5
    EXPECT_TRUE(o->is_close(b,5));
    EXPECT_FALSE(o->is_close(b,4));
}

// ---------------- notify multiple observers ----------------
TEST(ObserverTest, MultipleObservers) {
    auto o = createNPC(NPCType::Orc,"O1",0,0);
    auto b = createNPC(NPCType::Bear,"B1",0,0);
    auto obs1 = std::make_shared<TestObserver>();
    auto obs2 = std::make_shared<TestObserver>();
    o->subscribe(obs1);
    o->subscribe(obs2);

    o->notify_kill(b);
    EXPECT_EQ(obs1->logs.size(),1u);
    EXPECT_EQ(obs2->logs.size(),1u);
}

// ---------------- createNPCFromStream ----------------
TEST(CreateFromStreamTest, Basic) {
    std::istringstream ss("1 Orc1 10 20\n");
    auto npc = createNPCFromStream(ss);
    ASSERT_TRUE(npc);
    EXPECT_EQ(npc->type,NPCType::Orc);
    EXPECT_EQ(npc->name,"Orc1");
    EXPECT_EQ(npc->x,10);
    EXPECT_EQ(npc->y,20);
}

// ---------------- type_to_string ----------------
TEST(NPCTest, TypeToString) {
    EXPECT_EQ(type_to_string(NPCType::Orc),"Orc");
    EXPECT_EQ(type_to_string(NPCType::Bear),"Bear");
    EXPECT_EQ(type_to_string(NPCType::Squirrel),"Squirrel");
    EXPECT_EQ(type_to_string(NPCType::Unknown),"Unknown");
}

// ---------------- fight_round with no close NPCs ----------------
TEST(FightRoundTest, NoKillsIfFar) {
    auto o = createNPC(NPCType::Orc,"O1",0,0);
    auto b = createNPC(NPCType::Bear,"B1",100,100);
    std::vector<std::shared_ptr<NPC>> list{o,b};
    auto dead = fight_round(list,10);
    EXPECT_EQ(dead.size(),0u);
}

// ---------------- fight_round both die ----------------
TEST(FightRoundTest, BothDie) {
    auto o = createNPC(NPCType::Orc,"O1",0,0);
    auto o2 = createNPC(NPCType::Orc,"O2",0,0);
    std::vector<std::shared_ptr<NPC>> list{o,o2};
    auto dead = fight_round(list,1);

    // В старых тестах ожидалось, что убит только один, т.к. бой последовательный
    EXPECT_EQ(dead.size(),2u);
}

// ---------------- Observer does not record non-win ----------------
TEST(ObserverTest, NoLogOnLose) {
    auto o = createNPC(NPCType::Orc,"O1",0,0);
    auto b = createNPC(NPCType::Bear,"B1",0,0);
    auto obs = std::make_shared<TestObserver>();
    b->subscribe(obs);

    std::vector<std::shared_ptr<NPC>> list{o,b};
    fight_round(list,1);
    EXPECT_TRUE(obs->logs.empty());
}

// ---------------- SaveLoad multiple NPCs ----------------
TEST(SaveLoadTest, MultipleNPCs) {
    std::vector<std::shared_ptr<NPC>> list;
    list.push_back(createNPC(NPCType::Orc,"O1",1,1));
    list.push_back(createNPC(NPCType::Bear,"B1",2,2));
    list.push_back(createNPC(NPCType::Squirrel,"S1",3,3));

    std::string fname="tmp.txt";
    save_all(list,fname);

    auto loaded = load_all(fname);
    EXPECT_EQ(loaded.size(),3u);
    EXPECT_EQ(loaded[2]->type,NPCType::Squirrel);
    std::remove(fname.c_str());
}

// ---------------- MAIN ----------------
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
