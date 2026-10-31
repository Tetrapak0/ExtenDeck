#pragma once

#include "Client/Client.hpp"
#include "Config/State.hpp"
#include "Event/Event.hpp"
#include "GUI/DxWindow.hpp"
#include "Networking/Networking.hpp"
#include "Plugin/ApplicationPlugin.hpp"
#include "Server/Server.hpp"
#include "Plugin/PluginManager.hpp"

extern int main(int argc, char** argv);

namespace ExtenDeck {
class Deckastore {
    friend int ::main(int argc, char** argv);

    tasks running_tasks = tasks::None;
    statuses status = statuses::Starting;
    modes mode = modes::Server;

#ifdef _DEBUG
    bool singleInstance = true;
    void disable_sis();
public:
    bool isSingleInstanceEnforced() const;
private:
#endif
    uint64_t selected_client_id = DX_INVALID_UUID;

    json config;
    bool headless = false;

    static void reset();

    void set_mode(modes mode);
    int check_config(bool rerunSetup = false);
    int runSetup();
    
    Event::EventRegistry eventRegistry;
public:
    mutex lock;
    DxWindow window;
    Client::ClientManager clientManager;
    Event::EventBus eventBus{eventRegistry};
    Networking::NetworkManager networkManager;
    Server::Server server;
    Plugin::PluginManager pluginManager;
    Plugin::Plugin* applicationPlugin;

    ImFont* headerFont  = nullptr;
    ImFont* regularFont = nullptr;

    bool clientPropertiesShown = false;
    bool settingsShown         = false;

    void drawSettings();

    Client::Client* retrieve_current_client();

    const uint64_t& get_selected_uuid() const;
    void set_selected_uuid(uint64_t uuid);

    static Deckastore& get();

    void set_status(statuses status);
    const statuses& get_status() const;

    modes get_mode() const;
    bool isHeadless() const;

    const tasks& get_tasks() const;
    void add_task(tasks task);
    void remove_task(tasks task);

    ~Deckastore() = default;
private:
    Deckastore();
};
}
