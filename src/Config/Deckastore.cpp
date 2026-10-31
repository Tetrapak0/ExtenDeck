#include "Config/Deckastore.hpp"
#include "Config/Config.hpp"
#include "Plugin/ApplicationPlugin.hpp"
#include "SDK/GUIConstants.hpp"
#include "Utilities/FileDialog.hpp"

#include "ShortcutView/ShortcutViewPlugin.hpp"

#include <fstream>

namespace ExtenDeck {
Deckastore::Deckastore() : pluginManager(eventBus, mode, status, running_tasks) {
    applicationPlugin = pluginManager.registerPlugin(&applicationPluginDescriptor);
    assert(applicationPlugin);

    eventBus.registry.registerPrivateEventType("org.PoS.ExtenDeck.events.ClientConnected", sizeof(Event::EClientConnected),
                                                ExtenDeckSDK::DxECEPAll, applicationPlugin->plugin, [](ExtenDeckSDK::EventBase* event){
        new (event) Event::EClientConnected{};
    });
    eventBus.registry.registerEventType("org.PoS.ExtenDeck.events.ClientDisconnected",
                                         sizeof(Event::EClientDisconnected), ExtenDeckSDK::DxECEPImmediate, [](ExtenDeckSDK::EventBase* event){
        new (event) Event::EClientDisconnected{};
    });
    eventBus.registry.registerEventType("org.PoS.ExtenDeck.events.ClientDisconnectIssued", sizeof(Event::EClientDisconnectIssued),
                                        (ExtenDeckSDK::CallbackExecutionPolicy)(ExtenDeckSDK::DxECEPWorkerThread | ExtenDeckSDK::DxECEPSubscriberThread),
                                        [](ExtenDeckSDK::EventBase* event){
        new (event) Event::EClientDisconnectIssued{};
    });
    eventBus.registry.registerEventType("org.PoS.ExtenDeck.events.ServerConnected", sizeof(Event::EServerConnected), 
                                        ExtenDeckSDK::DxECEPAll, [](ExtenDeckSDK::EventBase* event){
        new (event) Event::EServerConnected{};
    }, [](ExtenDeckSDK::EventBase* event){
        ((Event::EServerConnected*)event)->~EServerConnected();
    });
    eventBus.registry.registerEventType("org.PoS.ExtenDeck.events.ServerDisconnected",sizeof(Event::EServerDisconnected),
                                        ExtenDeckSDK::DxECEPAll, [](ExtenDeckSDK::EventBase* event){
        new (event) Event::EServerDisconnected{};
    }, [](ExtenDeckSDK::EventBase* event){
        ((Event::EServerDisconnected*)event)->~EServerDisconnected();
    });

    eventBus.registry.registerPrivateEventType("org.PoS.ExtenDeck.events.ServerFound", sizeof(Event::EServerFound),
                                               ExtenDeckSDK::DxECEPAll, applicationPlugin->plugin, [](ExtenDeckSDK::EventBase* event){
        new (event) Event::EServerFound{};
    }, [](ExtenDeckSDK::EventBase* event){
        ((Event::EServerFound*)event)->~EServerFound();
    });
    eventBus.registry.registerPrivateEventType("org.PoS.ExtenDeck.events.TextureReadyToBind", sizeof(Event::ETextureReadyToBind),
                                           ExtenDeckSDK::DxECEPSubscriberThread, applicationPlugin->plugin, [](ExtenDeckSDK::EventBase* event){
        new (event) Event::ETextureReadyToBind{};
    });

    pluginManager.registerPlugin(&ShortcutView::pluginDescriptor);
}

void Deckastore::reset() {
    Deckastore::get().~Deckastore();
    new (&Deckastore::get()) Deckastore;
}

void Deckastore::set_mode(const modes _mode) {
    std::lock_guard _lock(this->lock);
    this->mode = _mode;
}
#ifdef _DEBUG
void Deckastore::disable_sis() {
    singleInstance = false;
}

bool Deckastore::isSingleInstanceEnforced() const {
    return singleInstance;
}
#endif
Deckastore& Deckastore::get() {
    static Deckastore self;
    return self;
}

Client::Client* Deckastore::retrieve_current_client() {
    return clientManager.retrieve_client(selected_client_id);
}

void Deckastore::drawSettings() {
    static int mode = static_cast<int>(get_mode());
    static bool discoverable = server.get_discoverable();
    static int port = server.get_port();
    if (!ImGui::IsPopupOpen("Settings")) {
        ImGui::OpenPopup("Settings");
        mode = static_cast<int>(get_mode());
        discoverable = server.get_discoverable();
        port = server.get_port();
    }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480, 360));
    if (ImGui::BeginPopupModal("Settings", nullptr, GUI::Constants::modalFlags)) {
        ImGui::Text("Select device mode:");
        ImGui::RadioButton("Server", &mode, 0);
        ImGui::BeginDisabled(mode);
        ImGui::SetCursorPosX(24);
        ImGui::Checkbox("Make this server discoverable on my network", &discoverable);
        ImGui::SetCursorPosX(24);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Server port: ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(256);
        static constexpr unsigned short step = 1;
        static constexpr unsigned short step_fast = 100;
        ImGui::InputScalar("##port", ImGuiDataType_U16, &port, &step, &step_fast);
        ImGui::EndDisabled();
        ImGui::RadioButton("Client", &mode, 1);
        ImGui::SetCursorPos(ImVec2(340, 326));
        if (ImGui::Button("Cancel", ImVec2(64, 26))) {
            mode = static_cast<int>(get_mode());
            discoverable = server.get_discoverable();
            port = server.get_port();
            ImGui::CloseCurrentPopup();
            settingsShown = false;
        }
        ImGui::SameLine();
        if (mode == static_cast<int>(get_mode()) && port == server.get_port()) {
            ImGui::BeginDisabled(discoverable == server.get_discoverable());
            if (ImGui::Button("Apply", ImVec2(64, 26))) {
                server.set_discoverable(discoverable);
                fs::path dkd_dir = get_config_dir();
                fs::create_directories(dkd_dir);
                config["server"]["discoverable"] = discoverable;
                std::ofstream writer(dkd_dir / "config.json");
                writer << config.dump(4);
                writer.close();
                ImGui::CloseCurrentPopup();
                settingsShown = false;
            }
            ImGui::EndDisabled();
        } else if (ImGui::Button("Restart", ImVec2(64, 26))) {
            config["mode"] = mode;
            config["server"]["discoverable"] = discoverable;
            config["server"]["port"] = port;
            fs::path dkd_dir = get_config_dir();
            fs::create_directories(dkd_dir);
            std::ofstream writer(dkd_dir / "config.json");
            writer << config.dump(4);
            writer.close();
            ImGui::CloseCurrentPopup();
            set_status(statuses::Restart);
            settingsShown = false;
        }
        ImGui::EndPopup();
    }
}

//int Deckastore::runSetup(); // Defined in Config/Setup.cpp

int Deckastore::check_config(bool rerunSetup) {
    fs::path dkd_dir = get_config_dir();

    if (!fs::exists(dkd_dir / "config.json") || rerunSetup) {
        set_mode(modes::Undefined);
        server.set_port(32018);
        server.set_discoverable(true);
        if (runSetup())
            return -1;
    }

    std::ifstream reader(dkd_dir / "config.json");
    config = json::parse(reader);
    if (config.is_discarded())
        return check_config(true);
    reader.close();

    if (config.contains("mode") && config["mode"].is_number_unsigned()) {
        modes mode = static_cast<modes>(config["mode"].get<uint64_t>());
        set_mode(mode);
        if (mode == modes::Server) {
            if (config.contains("server") && config["server"].is_object()) {
                if (config["server"].contains("discoverable") && config["server"]["discoverable"].is_boolean())
                    server.set_discoverable(config["server"]["discoverable"]);
                else
                    return check_config(true);
                if (config["server"].contains("port"))
                    server.set_port(config["server"]["port"].get<uint16_t>());
            } else {
                return check_config(true);
            }
        }
    } else {
        return check_config(true);
    }
    return 0;
}

const uint64_t& Deckastore::get_selected_uuid() const {
    return selected_client_id;
}
void Deckastore::set_selected_uuid(const uint64_t uuid) {
    lock.lock();
    selected_client_id = uuid;
    lock.unlock();
}

void Deckastore::set_status(const statuses status) {
    lock.lock();
    this->status = status;
    lock.unlock();
}
const statuses& Deckastore::get_status() const {
    return status;
}

modes Deckastore::get_mode() const {
    return mode;
}

bool Deckastore::isHeadless() const {
    return headless;
}

const tasks& Deckastore::get_tasks() const {
    return running_tasks;
}
void Deckastore::add_task(const tasks task) {
    running_tasks |= task;
}
void Deckastore::remove_task(const tasks task) {
    running_tasks &= ~task;
}
}
