#include "include/npc.h"
#include "include/game_utils.h"

#include <thread>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;


int main() {
    auto consoleObs = ConsoleObserver::get();
    auto fileObs = FileObserver::get("log.txt");

    // ---- NPCs ----
    std::vector<std::shared_ptr<NPC>> npcs;
    constexpr int NPC_COUNT = 50;

    for (int i = 0; i < NPC_COUNT; ++i) {
        NPCType t = random_type();
        std::string name;
        if (t == NPCType::Orc) name = "Orc_" + std::to_string(i+1);
        else if (t == NPCType::Bear) name = "Bear_" + std::to_string(i+1);
        else name = "Squirrel_" + std::to_string(i+1);

        auto npc = createNPC(
            t,
            name,
            random_coord(0, MAP_X),
            random_coord(0, MAP_Y)
        );

        npc->subscribe(consoleObs);
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

                int d = npc->get_move_distance();
                npc->move(
                    std::rand() % (2 * d + 1) - d,
                    std::rand() % (2 * d + 1) - d,
                    MAP_X,
                    MAP_Y
                );
            }

            for (size_t i = 0; i < npcs.size(); ++i)
                for (size_t j = i + 1; j < npcs.size(); ++j)
                    FightManager::instance().push({npcs[i], npcs[j]});

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
