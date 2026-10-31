#pragma once

#include "Networking/Networking.hpp"

#include <cstdint>

#include <Tetrapak0/NexusSockets.h>

namespace ExtenDeck::Server {
class Server {
    uint16_t port = 32018;
    bool discoverable = true;
public:
    void set_discoverable(bool d) {
        discoverable = d;
    }
    const bool& get_discoverable() const {
        return discoverable;
    }

    void set_port(uint16_t port) {
        this->port = port;
    }
    uint16_t get_port() const {
        return port;
    }
};

int run();
int start_discovery_service();
int server_gui_init();
}
