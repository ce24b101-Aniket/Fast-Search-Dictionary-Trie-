#include <array>
#include <iostream>

#include "api/server.hpp"
#include "db/database_manager.hpp"
#include "db/search_history_repository.hpp"
#include "db/word_repository.hpp"
#include "service/search_service.hpp"

namespace {

// The original project's civil/construction dataset, preserved as the
// seed dictionary -- kept as a nod to where this project started, not
// because it's structurally special to the engine in any way.
constexpr std::array<const char*, 21> kSeedWords = {
    "cement",   "concrete", "concreting", "column",  "conduit",   "beam",
    "bridge",   "brick",    "bitumen",    "boulder", "gravel",    "girder",
    "grout",    "granite",  "steel",      "sand",    "slab",      "sealant",
    "asphalt",  "aggregate", "admixture",
};

}  // namespace

int main(int argc, char** argv) {
    std::string dbPath = "fastsearch.db";
    int port = 8080;
    if (argc > 1) dbPath = argv[1];
    if (argc > 2) port = std::stoi(argv[2]);

    std::cout << "[startup] opening database: " << dbPath << "\n";
    fastsearch::DatabaseManager db(dbPath);
    db.migrate();

    fastsearch::WordRepository wordRepo(db);
    fastsearch::SearchHistoryRepository historyRepo(db);

    if (wordRepo.count() == 0) {
        std::cout << "[startup] empty database -- seeding " << kSeedWords.size()
                  << " words\n";
        for (const char* w : kSeedWords) {
            wordRepo.insert(w);
        }
    }

    std::cout << "[startup] loading dictionary and building Trie...\n";
    fastsearch::SearchService service(wordRepo, historyRepo);
    std::cout << "[startup] ready.\n";

    fastsearch::api::ApiServer server(service);
    std::cout << "[startup] listening on http://0.0.0.0:" << port << "\n";
    server.listen("0.0.0.0", port);

    return 0;
}
