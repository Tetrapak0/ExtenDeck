#include "Networking/Networking.hpp"
#include "Client/Client.hpp"

#include <Tetrapak0/NexusSockets.h>

#include <vector>

using std::vector;

namespace ExtenDeck::Networking {
int NetworkManager::query_interfaces() {
#ifdef _WIN32
    ULONG buflen = 15 * 1024;
    vector<char> buffer(buflen);
    auto* adapters = (IP_ADAPTER_ADDRESSES*)buffer.data();
    if (!GetAdaptersAddresses(AF_INET, NULL, nullptr, adapters, &buflen)) {
        for (auto* adapter = adapters; adapter; adapter = adapter->Next) {
            if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK || adapter->IfType == IF_TYPE_TUNNEL || adapter->OperStatus != IfOperStatusUp)
                continue;
            for (auto* unicast_addr = adapter->FirstUnicastAddress; unicast_addr; unicast_addr = unicast_addr->Next) {
                auto* saddr = (sockaddr_in*)unicast_addr->Address.lpSockaddr;
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &saddr->sin_addr, ip, sizeof(ip));
                interfaces.emplace_back(adapter->AdapterName, ip);
            }
        }
    }
#else
    ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == NX_SOCKET_ERROR)
        return -1;

    for (ifaddrs* ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
            continue;
        if (ifa->ifa_flags & IFF_LOOPBACK)
            continue;
        if (!(ifa->ifa_flags & IFF_UP))
            continue;
        if (!strncmp(ifa->ifa_name, "tun", 3) || !strncmp(ifa->ifa_name, "tap", 3) ||
            !strncmp(ifa->ifa_name, "p2p", 3) || !strncmp(ifa->ifa_name, "wifi-direct", 11))
            continue;
        void* addr = &((sockaddr_in*)ifa->ifa_addr)->sin_addr;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, addr, ip, sizeof(ip));
        interfaces.emplace_back(ifa->ifa_name, ip);
    }
    freeifaddrs(ifaddr);
#endif
    return interfaces.size();
}

vector<NetworkInterface>& NetworkManager::get_interfaces() {
    return interfaces;
}
}
