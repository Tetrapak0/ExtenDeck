#pragma once

#ifdef _WIN32
    #define CFG_VAR "APPDATA"
    #define CFG_REQ "ExtenDeck"
    #define USR_VAR "USERNAME"
#else
    #define CFG_VAR "HOME"
    #define CFG_REQ ".config/ExtenDeck"
    #define USR_VAR "LOGNAME"
#endif

#include <cstdint>
#include <filesystem>

namespace fs = std::filesystem;

namespace ExtenDeck {
    uint64_t generate_uuid();
    fs::path get_config_dir();
}