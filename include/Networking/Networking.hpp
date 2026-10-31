#pragma once

#include <Tetrapak0/NexusSockets.h>

#include <cassert>
#include <string>
#include <vector>

using std::string;
using std::vector;

#ifndef _WIN32
#ifdef NXS_LITTLE_ENDIAN
inline uint64_t htonll(uint64_t host) {
    return (uint64_t)htonl((uint32_t)(host >> 32)) |
           (uint64_t)htonl((uint32_t)host) << 32;
}
inline uint64_t ntohll(uint64_t net) {
    return htonll(net);
}
#else
inline uint64_t htonll(uint64_t host) {
    return host;
}
inline uint64_t ntohll(uint64_t net) {
    return net;
}
#endif
#endif

namespace ExtenDeck::Networking {
struct IPv4 {
    uint8_t byte1 = 0;
    uint8_t byte2 = 0;
    uint8_t byte3 = 0;
    uint8_t byte4 = 0;
    string to_string() const {
        return std::to_string(byte1) + '.' +
               std::to_string(byte2) + '.' +
               std::to_string(byte3) + '.' +
               std::to_string(byte4);
    }
    // terrible impl but under proper use should work fine
    void from_string(const string& ipv4) {
        assert(ipv4.length() <= 15);
        int periodPos = -1;
        int bytes_set = 0;
        uint32_t whole = 0;
        for (int i = 0; i <= ipv4.length(); ++i) {
            if (i != ipv4.length() && isdigit((uint8_t)ipv4[i])) continue;
            whole |= (uint32_t)strtol(ipv4.substr(periodPos+1, i-periodPos-1).c_str(), nullptr, 10) << ((3-bytes_set) * 8);
            periodPos = i;
            ++bytes_set;
        }
        byte1 = whole >> 24;
        assert(byte1 <= 255);
        byte2 = whole << 8 >> 24;
        assert(byte2 <= 255);
        byte3 = whole << 16 >> 24;
        assert(byte3 <= 255);
        byte4 = whole << 24 >> 24;
        assert(byte4 <= 255);
    }
    IPv4() = default;
    explicit IPv4(const string& ipv4) {
        from_string(ipv4);
    }
};

struct NetworkInterface {
    string name;
    IPv4 ipv4{};
    socket_t discovery_sock = NX_INVALID_SOCKET;
    socket_t listen_socket = NX_INVALID_SOCKET;
    NetworkInterface(string name, const char* addr) : name(std::move(name)), ipv4(addr) {}
    NetworkInterface(string name, const IPv4& addr) : name(std::move(name)), ipv4(addr) {}
    NetworkInterface() = default;
    ~NetworkInterface() {
        if (discovery_sock != NX_INVALID_SOCKET)
            nx_sock_close(discovery_sock);
        if (listen_socket != NX_INVALID_SOCKET)
            nx_sock_close(discovery_sock);
    }
};

class NetworkManager {
    vector<NetworkInterface> interfaces;
public:
    int query_interfaces();
    vector<NetworkInterface>& get_interfaces();
};
}
