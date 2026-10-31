#include "Server/DiscoveryService.hpp"
#include "Config/Config.hpp"
#include "Config/Deckastore.hpp"
#include "Server/Server.hpp"
#include "Networking/Networking.hpp"

#include <thread>
#include <vector>

using std::vector;

namespace ExtenDeck::Server {
int start_discovery_service() {
    Deckastore& dxstore = Deckastore::get();
    const statuses& dxstatus = dxstore.get_status();
    const bool& discoverable = dxstore.server.get_discoverable();
    dxstore.add_task(tasks::Discovery);
    uint16_t port = dxstore.server.get_port();

    sockaddr_in mcast_addr{};
    mcast_addr.sin_family = AF_INET;
    inet_pton(AF_INET, MCAST_IP, &mcast_addr.sin_addr.s_addr);
    mcast_addr.sin_port = htons(MCAST_PORT);

    for (auto& iface : dxstore.networkManager.get_interfaces()) {
        iface.discovery_sock = socket(AF_INET, SOCK_DGRAM, 0);
    #ifdef _DEBUG // Allow multiple instances
        int reuse = 1;
        setsockopt(iface.discovery_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
    #endif

        sockaddr_in loc_addr{};
        loc_addr.sin_family = AF_INET;
        inet_pton(AF_INET, iface.ipv4.to_string().c_str(), &loc_addr.sin_addr.s_addr);
        loc_addr.sin_port = htons(MCAST_PORT);
        ip_mreq mreq{};
        inet_pton(AF_INET, MCAST_IP, &mreq.imr_multiaddr.s_addr);
        inet_pton(AF_INET, iface.ipv4.to_string().c_str(), &mreq.imr_interface.s_addr);

        if (bind(iface.discovery_sock, (sockaddr*)&loc_addr, sizeof(loc_addr))) {
            nx_sock_close(iface.discovery_sock);
            iface.discovery_sock = NX_INVALID_SOCKET;
        }

        setsockopt(iface.discovery_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
    }

    char hn[253]; // max hostname on Linux
    const char* un = getenv(USR_VAR);
    gethostname(hn, 253);
    while (!static_cast<int>(dxstatus) && discoverable) {
        for (auto& iface : dxstore.networkManager.get_interfaces()) {
            if (iface.discovery_sock == NX_INVALID_SOCKET)
                continue;
            string msg = string(un) + "@" + hn + " (" + iface.ipv4.to_string() + ":" + std::to_string(port) + ")";
            sendto(iface.discovery_sock, msg.c_str(), msg.length(), 0, (sockaddr*)&mcast_addr, sizeof(mcast_addr));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    for (auto& iface : dxstore.networkManager.get_interfaces()) {
        nx_sock_close(iface.discovery_sock);
        iface.discovery_sock = NX_INVALID_SOCKET;
    }
    dxstore.remove_task(tasks::Discovery);
    return 0;
}
}
