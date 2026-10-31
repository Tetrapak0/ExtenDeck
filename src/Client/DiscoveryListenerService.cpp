#include "Server/DiscoveryService.hpp"
#include "Client/DiscoveryListenerService.hpp"
#include "Config/Deckastore.hpp"
#include "Event/Event.hpp"

#include <print>

namespace ExtenDeck {
namespace Networking {
ServerInfo::ServerInfo(string msg, uint64_t last_ping) : last_ping(last_ping), full(msg) {
    if (msg.empty())
        return;
    size_t at = std::find(msg.begin(), msg.end(), '@') - msg.begin();
    size_t sp = std::find(msg.begin(), msg.end(), '(') - msg.begin();
    size_t cl = std::find(msg.begin(), msg.end(), ':') - msg.begin();
    username = msg.substr(0, at);
    hostname = msg.substr(at+1, sp-at-2);
    ipv4     = msg.substr(sp+1, cl-sp-1);
    port     = strtol(msg.substr(cl+1, msg.length()-cl-2).c_str(), nullptr, 10);
}
void ServerInfo::print() {
    std::println("Username:   {}", username);
    std::println("Hostname:   {}", hostname);
    std::println("IP address: {}", ipv4);
    std::println("Port:       {}", port);
}
}

namespace Client {
void start_listening() {
    Deckastore& dxstore = Deckastore::get();
    dxstore.add_task(tasks::Discovery);
    const statuses& dxstatus = dxstore.get_status();
    Client* dxclient = dxstore.retrieve_current_client();

    vector<Networking::NetworkInterface>& interfaces = dxstore.networkManager.get_interfaces();

    sockaddr_in mcast_addr{};
    mcast_addr.sin_family = AF_INET;
    inet_pton(AF_INET, MCAST_IP, &mcast_addr.sin_addr.s_addr);
    mcast_addr.sin_port = htons(MCAST_PORT);

    vector<pollfd> pollees;
    pollees.reserve(interfaces.size());
    for (auto& iface : interfaces) {
        iface.discovery_sock = socket(AF_INET, SOCK_DGRAM, 0);

#ifdef _DEBUG // Allow multiple instances
        int reuse = 1;
        setsockopt(iface.discovery_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
    #ifndef _WIN32
        setsockopt(iface.discovery_sock, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse));
    #endif
#endif

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(MCAST_PORT);
        ip_mreq mreq{};
        inet_pton(AF_INET, MCAST_IP, &mreq.imr_multiaddr.s_addr);
        inet_pton(AF_INET, iface.ipv4.to_string().c_str(), &mreq.imr_interface.s_addr);

        if (bind(iface.discovery_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))) {
            nx_sock_close(iface.discovery_sock);
            iface.discovery_sock = NX_INVALID_SOCKET;
            continue;
        }

        setsockopt(iface.discovery_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&mreq), sizeof(mreq));

        pollees.emplace_back(iface.discovery_sock, POLLIN);
    }
    while (!dxclient->connection.isConnected() && dxstatus == statuses::Running) {
        int pres = poll(pollees.data(), interfaces.size(), 2000);
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (!pres)
            continue;
        if (pres == NX_SOCKET_ERROR)
            break;
        for (auto& p : pollees) {
            if (!p.revents)
                continue;
            char buf[BUFSIZE] = {};

            socklen_t addrlen = sizeof(mcast_addr);
            int res = recvfrom(p.fd, buf, BUFSIZE, 0, reinterpret_cast<sockaddr*>(&mcast_addr), &addrlen);
            if (res == NX_SOCKET_ERROR)
                continue;
            buf[res] = '\0';

            Networking::ServerInfo serverInfo(buf, now);
            auto event = (Event::EServerFound*)dxstore.eventBus.registry
                                                .createEvent(dxstore.eventBus.registry
                                                             .findPrivateDescriptorByName("org.PoS.ExtenDeck.events.ServerFound",
                                                                                          dxstore.applicationPlugin->plugin));
            event->serverInfo = serverInfo;
            dxstore.eventBus.postEvent(event);
            dxstore.eventBus.registry.releaseEvent(event);
            p.revents = 0;
        }
    }

    for (auto& iface : interfaces) {
        nx_sock_close(iface.discovery_sock);
        iface.discovery_sock = NX_INVALID_SOCKET;
    }
    dxstore.remove_task(tasks::Discovery);
}
}
}
