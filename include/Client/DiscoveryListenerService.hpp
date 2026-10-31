#pragma once

#include <Tetrapak0/NexusSockets.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace ExtenDeck {
namespace Networking {
struct ServerInfo {
    string hostname;
    string username;
    string ipv4;
    uint16_t port;
    uint64_t last_ping;
    string full;
    void print();
    ServerInfo(string msg, uint64_t last_ping);
    ServerInfo() = default;
    bool operator==(const ServerInfo& other) const {
        return full == other.full;
    }
};
}
/*
struct ServerInfo {
    IPv4 ip;
    uint16_t port;
    char username[256];
    char hostname[255];
    uint64_t last_ping;
    bool operator==(const ServerInfo& other) const {
        return ip.to_string() == other.ip.to_string() &&
               port == other.port && !strcmp(hostname, other.hostname);
    }
    inline string to_string() {
        return string(username) + "@" + string(hostname) +
               " (" + ip.to_string() + ':' + std::to_string(port) + ")";
    }
};
*/
namespace Client {
void start_listening();
}
}
