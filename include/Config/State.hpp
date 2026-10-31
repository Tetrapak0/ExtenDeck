#pragma once

#include <stdint.h>

namespace ExtenDeck {
enum class statuses : uint64_t {
    Running     = 0,
    Starting    = 1,
    Exiting     = 2,
    Restart     = 3,
};
enum class modes : uint64_t {
    Server      = 0,
    Client      = 1,
    Undefined   = 0xFFFF
};

enum class tasks : uint64_t {
    None            = 0,
    SingleInstance  = 1,
    Server          = 2,
    Client          = 2,
    Discovery       = 4,
    Setup           = 8
};

inline tasks operator|(tasks lhs, tasks rhs) {
    return static_cast<tasks>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs));
}
inline tasks& operator|=(tasks& lhs, const tasks rhs) {
    lhs = lhs | rhs;
    return lhs;
}
inline tasks operator&(tasks lhs, tasks rhs) {
    return static_cast<tasks>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs));
}
inline tasks& operator&=(tasks& lhs, const tasks rhs) {
    lhs = lhs & rhs;
    return lhs;
}
inline tasks operator~(tasks rhs) {
    return static_cast<tasks>(~static_cast<uint64_t>(rhs));
}
}
