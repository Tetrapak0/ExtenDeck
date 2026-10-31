#include "Config/Deckastore.hpp"
#include "Config/Config.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <random>

namespace fs = std::filesystem;
using std::ifstream;
using std::string;

namespace ExtenDeck {
fs::path get_config_dir() {
    return fs::path(getenv(CFG_VAR)) / CFG_REQ;
}

uint64_t generate_uuid() {
    std::random_device rand_dev;
    std::mt19937_64 generator(rand_dev());
    std::uniform_int_distribution<uint64_t> distributor(1, UINT64_MAX);

    return distributor(generator);
}
}
