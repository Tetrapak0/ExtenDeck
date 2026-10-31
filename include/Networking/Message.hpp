#pragma once

#include <cstdint>

namespace ExtenDeck::Networking {
struct MessageHeader {
    uint64_t len;
    uint64_t pluginID;
    uint64_t messageID;
};
}
