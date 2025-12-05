#include "include/npc.h"
#include "include/orc.h"
#include "include/squirrel.h"
#include "include/bear.h"
#include "game_utils.h"

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
        
        auto p = createNPC(t, name, random_coord(), random_coord());
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
        auto dead = fight_round(loaded, distance);
        for (auto &d : dead)
            loaded.erase(std::remove(loaded.begin(), loaded.end(), d), loaded.end());
        total_killed += static_cast<int>(dead.size());
    }

    print_all(loaded);
    return 0;
}
