#include "include/npc.h"
#include "include/game_utils.h"

#include <thread>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;


int main() {
    // auto consoleObs = ConsoleObserver::get();
    auto fileObs = FileObserver::get("log.txt");

    // ---- NPCs ----
    std::vector<std::shared_ptr<NPC>> npcs;
    constexpr int NPC_COUNT = 50;

    for (int i = 0; i < NPC_COUNT; ++i) {
        NPCType t = random_type();
        std::string name =
            (t == NPCType::Orc ? "Orc_" :
            (t == NPCType::Bear ? "Bear_" : "Squirrel_")) + std::to_string(i);

        auto npc = createNPC(
            t,
            name,
            random_coord(0, MAP_X),
            random_coord(0, MAP_Y)
        );

        // npc->subscribe(consoleObs);
        npc->subscribe(fileObs);
        npcs.push_back(npc);
    }

    print_all(npcs);

    std::atomic<bool> running{true};

    // ---- Fight thread ----
    std::thread fight_thread(std::ref(FightManager::instance()));

    // ---- Move + detect ----
    std::thread move_thread([&]() {
        while (running) {
            for (auto& npc : npcs) {
                if (!npc->is_alive()) continue;

                int d = move_distance(npc->type);
                npc->move(
                    std::rand() % (2 * d + 1) - d,
                    std::rand() % (2 * d + 1) - d,
                    MAP_X,
                    MAP_Y
                );
            }

            for (auto& a : npcs)
                for (auto& b : npcs)
                    if (a != b &&
                        a->is_alive() &&
                        b->is_alive() &&
                        a->is_close(b, kill_distance(a->type)))
                        FightManager::instance().push({a, b});

            std::this_thread::sleep_for(10ms);
        }
    });

    // ---- Map thread (1 sec) ----
    std::thread print_thread([&]() {
        while (running) {
            draw_map(npcs);
            std::this_thread::sleep_for(1s);
        }
    });

    // ---- Game duration ----
    std::this_thread::sleep_for(30s);
    running = false;

    // ---- Finish ----
    move_thread.join();
    print_thread.join();

    FightManager::instance().stop();
    fight_thread.join();

    print_survivors(npcs);
    return 0;
}
