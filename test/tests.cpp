#include <gtest/gtest.h>
#include <memory>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <thread>
#include <chrono>
#include <mutex>
#include <sstream>

#include "../include/npc.h"
#include "../include/orc.h"
#include "../include/squirrel.h"
#include "../include/bear.h"
#include "../include/druid.h"
#include "../include/game_utils.h"

using namespace std::chrono_literals;

// ======================================================
// Observers
// ======================================================
struct TestObserver : public IInteractionObserver {
    struct Event {
        std::string actor;
        std::string target;
        InteractionOutcome outcome;
    };

    std::vector<Event> events;
    std::mutex mtx;

    void on_interaction(const std::shared_ptr<NPC>& a,
                  const std::shared_ptr<NPC>& d,
                  InteractionOutcome o) override {
        std::lock_guard<std::mutex> lck(mtx);
        events.push_back({a->name, d->name, o});
    }
};

TEST(ObserverTest, MultipleObserversNotified) {
    auto o = createNPC(NPCType::Orc,"O",0,0);
    auto b = createNPC(NPCType::Bear,"B",0,0);

    auto obs1 = std::make_shared<TestObserver>();
    auto obs2 = std::make_shared<TestObserver>();

    b->subscribe(obs1);
    b->subscribe(obs2);

    std::thread t(std::ref(InteractionManager::instance()));
    InteractionManager::instance().push({o,b});

    std::this_thread::sleep_for(200ms);
    InteractionManager::instance().stop();
    t.join();

    EXPECT_EQ(obs1->events.size(), obs2->events.size());
}

TEST(ObserverTest, ObserverNotCalledIfNoInteraction) {
    auto o = createNPC(NPCType::Orc,"O",0,0);
    auto obs = std::make_shared<TestObserver>();
    o->subscribe(obs);
    EXPECT_TRUE(obs->events.empty());
}

// ======================================================
// NPC creation
// ======================================================
TEST(NPCTest, CreateBear) {
    auto b = createNPC(NPCType::Bear, "B", 0, 0);
    EXPECT_EQ(b->type, NPCType::Bear);
    EXPECT_EQ(b->name, "B");
    EXPECT_EQ(b->x, 0);
    EXPECT_EQ(b->y, 0);
}

TEST(NPCTest, CreateOrc) {
    auto o = createNPC(NPCType::Orc, "O", 1, 1);
    EXPECT_EQ(o->type, NPCType::Orc);
    EXPECT_EQ(o->name, "O");
    EXPECT_EQ(o->x, 1);
    EXPECT_EQ(o->y, 1);
}

TEST(NPCTest, CreateSquirrel) {
    auto s = createNPC(NPCType::Squirrel, "S", 2, 2);
    EXPECT_EQ(s->type, NPCType::Squirrel);
    EXPECT_EQ(s->name, "S");
    EXPECT_EQ(s->x, 2);
    EXPECT_EQ(s->y, 2);
}

TEST(NPCTest, NPCTest_CreateDruid_Test) {
    auto d = createNPC(NPCType::Druid, "D", 3, 3);
    EXPECT_EQ(d->type, NPCType::Druid);
    EXPECT_EQ(d->name, "D");
    EXPECT_EQ(d->x, 3);
    EXPECT_EQ(d->y, 3);
}

TEST(NPCTest, EmptyNameAllowed) {
    auto o = createNPC(NPCType::Orc,"",0,0);
    EXPECT_EQ(o->name,"");
}

TEST(NPCTest, LongName) {
    std::string name(1000,'x');
    auto b = createNPC(NPCType::Bear,name,0,0);
    EXPECT_EQ(b->name.size(),1000u);
}

// ======================================================
// Distance
// ======================================================
TEST(DistanceTest, IsCloseExact) {
    auto a = createNPC(NPCType::Orc,"A",0,0);
    auto b = createNPC(NPCType::Bear,"B",6,8);
    EXPECT_TRUE(a->is_close(b,a->get_interaction_distance()));
    EXPECT_FALSE(a->is_close(b,a->get_interaction_distance()-1));
}

TEST(DistanceTest, KillDistance) {
    EXPECT_EQ(createNPC(NPCType::Orc,"O",0,0)->get_interaction_distance(), 10);
    EXPECT_EQ(createNPC(NPCType::Bear,"B",0,0)->get_interaction_distance(), 10);
    EXPECT_EQ(createNPC(NPCType::Squirrel,"S",0,0)->get_interaction_distance(), 5);
}

// ======================================================
// Kill rules (DETERMINISTIC)
// ======================================================
TEST(CanKillTest, Rule1) {
    auto actor = createNPC(NPCType::Orc, "O1", 0, 0);
    auto target = createNPC(NPCType::Orc, "O2", 0, 0);
    AttackVisitor v(actor);
    InteractionOutcome outcome = target->accept(v);
    EXPECT_NE(outcome, InteractionOutcome::NoInteraction);
}

TEST(CanKillTest, Rule2) {
    auto actor = createNPC(NPCType::Orc, "O", 0, 0);
    auto target = createNPC(NPCType::Bear, "B", 0, 0);
    AttackVisitor v(actor);
    InteractionOutcome outcome = target->accept(v);
    EXPECT_NE(outcome, InteractionOutcome::NoInteraction);
}

TEST(CanKillTest, Rule3) {
    auto actor = createNPC(NPCType::Orc, "O", 0, 0);
    auto target = createNPC(NPCType::Druid, "D", 0, 0);
    AttackVisitor v(actor);
    InteractionOutcome outcome = target->accept(v);
    EXPECT_NE(outcome, InteractionOutcome::NoInteraction);
}

TEST(CanKillTest, Rule4) {
    auto actor = createNPC(NPCType::Bear, "B", 0, 0);
    auto target = createNPC(NPCType::Squirrel, "S", 0, 0);
    AttackVisitor v(actor);
    InteractionOutcome outcome = target->accept(v);
    EXPECT_NE(outcome, InteractionOutcome::NoInteraction);
}

// ======================================================
// Support rules (DETERMINISTIC)
// ======================================================
TEST(DruidSupportTest, HealsDeadBear) {
    auto druid = createNPC(NPCType::Druid, "D", 0, 0);
    auto bear  = createNPC(NPCType::Bear,  "B",  1, 1);

    bear->must_die();
    ASSERT_FALSE(bear->is_alive());

    SupportVisitor visitor(druid);
    auto outcome = bear->accept(visitor);

    EXPECT_EQ(outcome, InteractionOutcome::TargetHealed);

    bear->heal();
    EXPECT_TRUE(bear->is_alive());
}

TEST(DruidSupportTest, HealsDeadSquirrel) {
    auto druid = createNPC(NPCType::Druid, "D", 0, 0);
    auto squirrel  = createNPC(NPCType::Squirrel,  "S",  1, 1);

    squirrel->must_die();
    ASSERT_FALSE(squirrel->is_alive());

    SupportVisitor visitor(druid);
    auto outcome = squirrel->accept(visitor);

    EXPECT_EQ(outcome, InteractionOutcome::TargetHealed);

    squirrel->heal();
    EXPECT_TRUE(squirrel->is_alive());
}

TEST(DruidSupportTest, HealsDeadOrc) {
    auto druid = createNPC(NPCType::Druid, "D", 0, 0);
    auto orc  = createNPC(NPCType::Orc,  "O",  1, 1);

    orc->must_die();
    ASSERT_FALSE(orc->is_alive());

    SupportVisitor visitor(druid);
    auto outcome = orc->accept(visitor);

    EXPECT_EQ(outcome, InteractionOutcome::NoInteraction);
    EXPECT_FALSE(orc->is_alive());
}

// ======================================================
// Visitor (logic only)
// ======================================================
struct DummyVisitor : IInteractionVisitor {
    InteractionOutcome visit(Orc&) override { return InteractionOutcome::TargetKilled; }
    InteractionOutcome visit(Bear&) override { return InteractionOutcome::TargetEscaped; }
    InteractionOutcome visit(Squirrel&) override { return InteractionOutcome::TargetHealed; }
    InteractionOutcome visit(Druid&) override { return InteractionOutcome::NoInteraction; }
};

TEST(VisitorTest, VisitOrc) {
    auto o = createNPC(NPCType::Orc,"O",0,0);
    DummyVisitor v;
    EXPECT_EQ(o->accept(v), InteractionOutcome::TargetKilled);
}

TEST(VisitorTest, VisitBear) {
    auto b = createNPC(NPCType::Bear,"B",0,0);
    DummyVisitor v;
    EXPECT_EQ(b->accept(v), InteractionOutcome::TargetEscaped);
}

TEST(VisitorTest, VisitSquirrel) {
    auto s = createNPC(NPCType::Squirrel,"S",0,0);
    DummyVisitor v;
    EXPECT_EQ(s->accept(v), InteractionOutcome::TargetHealed);
}

TEST(VisitorTest, VisitDruid) {
    auto d = createNPC(NPCType::Druid,"D",0,0);
    DummyVisitor v;
    EXPECT_EQ(d->accept(v), InteractionOutcome::NoInteraction);
}

// ======================================================
// Save / Load
// ======================================================
TEST(SaveLoadTest, SaveLoad) {
    std::vector<std::shared_ptr<NPC>> list{
        createNPC(NPCType::Orc,"O",1,1),
        createNPC(NPCType::Bear,"B",2,2)
    };

    save_all(list,"tmp.txt");
    auto loaded = load_all("tmp.txt");

    EXPECT_EQ(loaded.size(),2u);
    EXPECT_EQ(loaded[0]->name,"O");
    std::remove("tmp.txt");
}

TEST(SaveLoadTest, SaveLoadEmpty) {
    std::vector<std::shared_ptr<NPC>> empty;
    save_all(empty,"empty.txt");

    auto loaded = load_all("empty.txt");
    EXPECT_TRUE(loaded.empty());

    std::remove("empty.txt");
}

// ======================================================
// InteractionManager
// ======================================================
TEST(InteractionManagerTest, NoInteractionIfVisitorNotApplicable) {
    auto o = createNPC(NPCType::Orc,"O",0,0);
    auto s = createNPC(NPCType::Squirrel,"S",0,0);

    auto obs = std::make_shared<TestObserver>();
    s->subscribe(obs);

    std::thread t(std::ref(InteractionManager::instance()));
    InteractionManager::instance().push({o,s});

    std::this_thread::sleep_for(200ms);
    InteractionManager::instance().stop();
    t.join();

    EXPECT_TRUE(obs->events.empty());
}

// ======================================================
// NPC Life
// ======================================================
TEST(NPCLifeTest, AliveByDefault) {
    auto o = createNPC(NPCType::Orc,"O",0,0);
    EXPECT_TRUE(o->is_alive());
}

TEST(NPCLifeTest, MustDieMakesDead) {
    auto b = createNPC(NPCType::Bear,"B",0,0);
    b->must_die();
    EXPECT_FALSE(b->is_alive());
}

TEST(NPCLifeTest, MustDieIdempotent) {
    auto s = createNPC(NPCType::Squirrel,"S",0,0);
    s->must_die();
    s->must_die();
    EXPECT_FALSE(s->is_alive());
}

// ======================================================
// Movement
// ======================================================
TEST(MoveTest, CannotExceedMoveDistance) {
    auto o = createNPC(NPCType::Orc,"O",0,0);
    int md = o->get_move_distance();
    o->move(md+10, md+10, 100, 100);
    auto [x,y] = o->position();
    EXPECT_GE(x, 0);
    EXPECT_GE(y, 0);
}

TEST(MoveTest, MaxMoveBounds) {
    auto b = createNPC(NPCType::Bear,"B",0,0);
    int md = b->get_move_distance();
    b->move(md, md, 50, 50);
    auto [x,y] = b->position();
    EXPECT_LE(x, 50);
    EXPECT_LE(y, 50);
}

// ======================================================
// MAIN
// ======================================================
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
