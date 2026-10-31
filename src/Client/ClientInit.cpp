#include "Client/Client.hpp"
#include "Client/DiscoveryListenerService.hpp"
#include "Config/Config.hpp"
#include "Config/Deckastore.hpp"
#include "SDK/SDK.h"

#include <fstream>
#include <string>
#include <thread>

using std::thread;
using std::string;

namespace ExtenDeck::Client {
void begin_receiver_loop() {
    Deckastore& dxstore = Deckastore::get();
    dxstore.add_task(tasks::Client);
    const statuses& dxstatus = dxstore.get_status();
    Client* dxclient = dxstore.retrieve_current_client();

    dxclient->communicate();

    dxclient->connection = Networking::Connection(NX_INVALID_SOCKET);
    ExtenDeckSDK::EventBase* event = dxstore.eventBus.registry.createEvent("org.PoS.ExtenDeck.events.ServerDisconnected");
    dxstore.eventBus.postEvent(event);
    dxstore.eventBus.registry.releaseEvent(event);
    dxstore.remove_task(tasks::Client);
}

void begin_client_loop() {
    Deckastore& dxstore = Deckastore::get();
    const statuses& dxstatus = dxstore.get_status();
    while (dxstatus == statuses::Running) {
        if (!dxstore.retrieve_current_client()->connection.isConnected())
            start_listening();
        else
            begin_receiver_loop();
    }
}

uint64_t find_uuid() {
    fs::path dkd_dir = get_config_dir();
    fs::create_directories(dkd_dir);
    uint64_t uuid = generate_uuid();
    if (fs::exists(dkd_dir / "config.json")) {
        std::ifstream reader(dkd_dir / "config.json");
        json cfg = json::parse(reader);
        reader.close();
        if (cfg.contains("uuid") && cfg["uuid"].is_number_unsigned())
            return cfg["uuid"].get<uint64_t>();
        std::ofstream writer(dkd_dir / "config.json");
        cfg["uuid"] = uuid;
        writer << cfg.dump(4);
        writer.close();
        return uuid;
    }
    std::ofstream writer(dkd_dir / "config.json");
    json cfg;
    cfg["uuid"] = uuid;
    writer << cfg.dump(4);
    writer.close();
    return uuid;
}

int run() {
    Deckastore& dxstore = Deckastore::get();
    // HACK: // FIXME:
#ifdef _DEBUG
    if (dxstore.window.create("ExtenDeck", {1280, 720}, 0)) {
#else
    if (dxstore.window.create("ExtenDeck", {1280, 720}, 1)) {
#endif
        dxstore.set_status(statuses::Exiting);
        return -1;
    }

    Event::ThreadLocalEventBus localEventBus(dxstore.eventBus);
    std::unique_ptr<Event::EventBus::SubscriptionToken> textureReadyToBindToken;
    textureReadyToBindToken = localEventBus.eventBus.subscribeToEvent(
                                    dxstore.eventBus.registry.findPrivateDescriptorByName(
                                        "org.PoS.ExtenDeck.events.TextureReadyToBind", dxstore.applicationPlugin->plugin),
                                    [](const ExtenDeckSDK::EventBase* event, void* ctx) {
        ((Event::ETextureReadyToBind*)event)->texture->bind_to_context();
    }, nullptr, ExtenDeckSDK::DxECEPSubscriberThread); // FIXME: THIS SHIT WILL KICK THE BUCKET IF THE TEXTURE IS DESTROYED!!!!!!!

    const uint64_t uuid = find_uuid();
    dxstore.clientManager.insert_client(uuid, NX_INVALID_SOCKET);
    dxstore.set_selected_uuid(uuid);

    thread t_client(begin_client_loop);

    client_gui_init();

    t_client.join();

    dxstore.clientManager.disconnect_client(uuid, dxstore.clientManager.retrieve_client(uuid)->disconnectReason);

    return 0;
}
}
