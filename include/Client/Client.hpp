#pragma once

#define DX_INVALID_UUID 0

#include <mutex>
#include <string>
#include <vector>

#include <jsonhpp/json.hpp>

#include "Config/Profile.hpp"
#include "Networking/Connection.hpp"

using std::mutex;
using std::shared_ptr;
using std::string;
using std::unordered_map;
using std::vector;

using json = nlohmann::json;

namespace ExtenDeck::Client {
enum class ClientDisconnectReason {
    ConnectionClosed,
    DisconnectIssued
};

class Client {
    uint64_t m_uuid;
    string nickname;
    json config{};
    string m_nickname = nickname;
    mutex lock;
public:
    ClientDisconnectReason disconnectReason = ClientDisconnectReason::ConnectionClosed;
    uint64_t current_profile = 0;
    std::unordered_map<uint64_t, Profile> profiles;

    Networking::Connection connection;
    void communicate();

    int send(const ExtenDeckSDK::Message& msg, ExtenDeckSDK::PluginBase& sender);
    bool isConnected() const;

    uint64_t get_uuid() const;

    void set_nickname(const string& nick);
    const string& get_nickname() const;

    Profile& getCurrentProfile();

    json& get_config();
    void create_config();
    void write_config() const;
    void update_config();

    void draw_properties();

    Client() = delete;
    explicit Client(uint64_t uuid, socket_t socket);
    ~Client();
};

class ClientManager {
    unordered_map<uint64_t, Client> clients;
public:
    std::mutex mutex;
    Client* retrieve_client(uint64_t uuid);
    bool client_exists(uint64_t uuid) const;
    Client* insert_client(uint64_t uuid, socket_t socket);
    void disconnect_client(uint64_t uuid, ClientDisconnectReason reason);
    void request_disconnect(uint64_t uuid);
    uint64_t getNumOfClients() const;
    const unordered_map<uint64_t, Client>& getClientsMap();
};

int run();
uint64_t construct_header(uint32_t msg_len, uint32_t type);
int client_gui_init();
}
