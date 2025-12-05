#include "include/npc.h"
#include "include/orc.h"
#include "include/squirrel.h"
#include "include/bear.h"
#include "game_utils.h"

#include <algorithm>

int main() {
    auto consoleObs = std::make_shared<ConsoleObserver>();
    auto fileObs = std::make_shared<FileObserver>("log.txt");

    std::vector<std::shared_ptr<NPC>> list;
    const int START = 100;
    for (int i = 0; i < START; ++i) {
        NPCType t = random_type();
        std::string name;
        if (t == NPCType::Orc) name = "Orc_" + std::to_string(i+1);
        else if (t == NPCType::Bear) name = "Bear_" + std::to_string(i+1);
        else name = "Squirrel_" + std::to_string(i+1);
        
        int x = random_coord(0, 500);
        int y = random_coord(0, 500);
        auto p = createNPC(t, name, x, y);
        if (p) {
            p->subscribe(consoleObs);
            p->subscribe(fileObs);
            list.push_back(p);
        }
    }

    print_all(list);
    save_all(list, "npcs.txt");
    std::cout << "Saved to npcs.txt\n";

    auto loaded = load_all("npcs.txt");
    for (auto &p : loaded) {
        p->subscribe(consoleObs);
        p->subscribe(fileObs);
    }
    std::cout << "Loaded " << loaded.size() << " NPCs\n";

    int total_killed = 0;
    for (int distance = 20; distance <= 200 && !loaded.empty(); distance += 20) {
        std::cout << "\n";
        auto dead = fight_round(loaded, distance);
        for (auto &d : dead) {
            auto it = std::find(loaded.begin(), loaded.end(), d);
            if (it != loaded.end()) loaded.erase(it);
        }
        std::cout << "Distance=" << distance << " killed=" << dead.size() << " remaining=" << loaded.size() << "\n";
        total_killed += static_cast<int>(dead.size());
    }

    std::cout << "Fight ended. Total killed: " << total_killed << "\n";
    print_all(loaded);
    return 0;
}
