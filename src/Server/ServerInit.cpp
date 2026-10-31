#include "Server/Server.hpp"
#include "Client/Client.hpp"
#include "Config/Deckastore.hpp"

#include <print>
#include <thread>

using std::thread;
using std::vector;

namespace ExtenDeck::Server {
int begin_comm_loop(Client::Client* client) {
    if (!client)
        return -1;

    client->communicate();

    Deckastore::get().clientManager.disconnect_client(client->get_uuid(), client->disconnectReason);
    return 0;
}

int begin_listen_loop() {
    Deckastore& dxstore = Deckastore::get();
    dxstore.add_task(tasks::Server);
    vector<Networking::NetworkInterface>& interfaces = dxstore.networkManager.get_interfaces();

    vector<std::unique_ptr<thread>> client_threads;

    vector<pollfd> pollees(interfaces.size());
    for (int i = 0; i < interfaces.size(); ++i) {
        pollees[i].events = POLLIN;
        pollees[i].fd = interfaces[i].listen_socket;
    }

    while (dxstore.get_status() == statuses::Running) {
        const int res = poll(pollees.data(), interfaces.size(), 2000);
        if (!res)
            continue;
        if (res == NX_SOCKET_ERROR)
            break;
        printf("Connection request(s) received.\n");
        for (auto& p : pollees) {
            if (p.revents & POLLIN) {
                p.revents = 0;
                socket_t sock = accept(p.fd, nullptr, nullptr);;
                if (sock == NX_INVALID_SOCKET) {
                    std::println("accept() failed.");
                    continue;
                }
                uint64_t uuid = DX_INVALID_UUID;
                int recvRes = recvn(sock, &uuid, sizeof(uint64_t), 0);
                if (recvRes == NX_SOCKET_ERROR) {
                    std::println("recvn() failed.");
                    nx_sock_close(sock);
                    continue;
                }
                uuid = ntohll(uuid);
                if (Client::Client* client = dxstore.clientManager.insert_client(uuid, sock))
                    client_threads.push_back(std::make_unique<thread>(begin_comm_loop, client));
        }
        }
    }
    for (auto& client : client_threads)
        client->join();
    for (auto& iface : interfaces) {
        if (iface.listen_socket != NX_INVALID_SOCKET)
            nx_sock_close(iface.listen_socket);
    }
    dxstore.remove_task(tasks::Server);
    return 0;
}

socket_t create_socket(const Networking::NetworkInterface& iface) {
    int res = 0;

    socket_t listen_socket = NX_INVALID_SOCKET;

    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == NX_INVALID_SOCKET) {
        fprintf(stderr, "socket() failed.\n");
        return NX_INVALID_SOCKET;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(Deckastore::get().server.get_port());
    inet_pton(AF_INET, iface.ipv4.to_string().c_str(), &addr.sin_addr.s_addr);

    res = bind(listen_socket, (sockaddr*)&addr, sizeof(addr));
    if (res == NX_SOCKET_ERROR) {
        fprintf(stderr, "bind() failed.\n");
        return NX_INVALID_SOCKET;
    }

    res = listen(listen_socket, SOMAXCONN);
    if (res == NX_SOCKET_ERROR) {
        fprintf(stderr, "listen() failed.\n");
        return NX_INVALID_SOCKET;
    }

    return listen_socket;
}

int run() {
    Deckastore& dxstore = Deckastore::get();
    vector<Networking::NetworkInterface>& interfaces = dxstore.networkManager.get_interfaces();
    for (auto& iface : interfaces) {
        iface.listen_socket = create_socket(iface);
        if (iface.listen_socket == NX_INVALID_SOCKET) {
            fprintf(stderr, "Invalid socket was returned.\n");
            dxstore.remove_task(tasks::Server);
            dxstore.set_status(statuses::Exiting);
            return -1;
        }
    }
    thread t_server(begin_listen_loop);
    thread t_discovery([]{
        Deckastore& dxstore = Deckastore::get();
        const bool& discovery = dxstore.server.get_discoverable();
        // Allows discovery to be toggled without a restart
        while (dxstore.get_status() == statuses::Running) {
            if (discovery) start_discovery_service();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });

    if (!dxstore.isHeadless()) {
        // thread createTrayIcon;
        server_gui_init();
    }

    t_discovery.join();
    t_server.join();

    return 0;
}
}
