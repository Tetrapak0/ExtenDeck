#pragma once

#include <Tetrapak0/NexusSockets.h>

#include "SDK/SDK.h"

#include <cstdint>

namespace ExtenDeck {
namespace Client {
class Client;
}
namespace Networking {
class Connection {
    friend class Client::Client;
    socket_t socket = NX_INVALID_SOCKET;
    int res = 0;
    int send_res = 0;
public:
    int send(const char* data, uint64_t len);
    int send(const ExtenDeckSDK::Message& msg, const ExtenDeckSDK::PluginBase* plugin);
    bool isConnected() const;
    explicit Connection(socket_t socket = NX_INVALID_SOCKET) : socket(socket) {}
    ~Connection() {
        if (socket != NX_INVALID_SOCKET) {
            nx_sock_close(socket);
            socket = NX_INVALID_SOCKET;
        }
    }
};
}
}
