#include "Client/Client.hpp"
#include "Config/Deckastore.hpp"
#include "Networking/Message.hpp"
#include "SDK/GUIConstants.hpp"
#include "SDK/SDK.h"

#include <fstream>
#include <print>

namespace ExtenDeck::Client {
Client::Client(const uint64_t uuid, const socket_t socket) : m_uuid(uuid), connection(socket) {
    std::unique_lock clientLock(lock);

    fs::path dkd_dir = get_config_dir();
    fs::create_directories(dkd_dir);
    fs::path cfg_file(dkd_dir / (std::to_string(m_uuid)+".json"));
    if (!fs::exists(cfg_file))
        create_config();

    std::ifstream reader(dkd_dir / cfg_file);
    config = json::parse(reader, nullptr, false, true);
    reader.close();
    if (config.is_discarded())
        create_config();

    if (config.contains("nickname")) {
        nickname = config["nickname"];
        m_nickname = nickname;
    }
    if (!config.contains("profiles") || !config["profiles"].is_array() || config["profiles"].empty())
        create_config();
    bool haveCurrentProfile = false;
    if (config.contains("last_profile") && config["last_profile"].is_number_unsigned()) {
        current_profile = config["last_profile"].get<uint64_t>();
        haveCurrentProfile = true;
    }
    for (auto& profileConfig : this->config["profiles"]) {
        uint64_t profileUUID;
        if (profileConfig.contains("uuid") && profileConfig["uuid"].is_number_unsigned())
            profileUUID = profileConfig["uuid"].get<uint64_t>();
        else
            profileConfig["uuid"] = profileUUID = generate_uuid();
        if (!haveCurrentProfile) {
            current_profile = profileUUID;
            haveCurrentProfile = true;
        }
    }
    if (Deckastore::get().get_mode() == modes::Server) {
        write_config();
        update_config();
    }
    for (auto& profileConfig : this->config["profiles"]) {
        profiles.try_emplace(profileConfig["uuid"].get<uint64_t>(), *this, &profileConfig);
    }
    // TODO: Let's maybe not nuke the entire config in case of an error
}

void Client::communicate() {
    Deckastore& dxstore = Deckastore::get();
    pollfd pole{connection.socket, POLLIN, 0};
    connection.res = 1;
    connection.send_res = 1;
    do {
        if (disconnectReason == ClientDisconnectReason::DisconnectIssued)
            break;

        int res = poll(&pole, 1, 500);
        if (!res)
            continue;
        if (res == NX_SOCKET_ERROR || pole.revents & POLLERR || pole.revents & POLLHUP)
            break;

        Networking::MessageHeader header{};
        connection.res = recvn(connection.socket, &header, sizeof(header), 0);
        if (connection.res <= 0) {
#ifdef _WIN32
            std::println("{}", WSAGetLastError());
#else
            std::println("{}", strerror(errno));
#endif
            break;
        }
        header.len = ntohll(header.len);
        header.pluginID = ntohll(header.pluginID);
        header.messageID = ntohll(header.messageID);
        
        std::println("Message size: {}", header.len);
        std::println("Plugin ID: {}", header.pluginID);
        
        string buf(header.len, '\0');
        
        connection.res = recvn(connection.socket, buf.data(), header.len, 0);
        if (connection.res <= 0) {
#ifdef _WIN32
            std::println("{}", WSAGetLastError());
#else
            std::println("{}", strerror(errno));
#endif
            break;
        }

        dxstore.lock.lock();
        const unordered_map<uint64_t, Plugin::Plugin>& pluginMap = dxstore.pluginManager.getPluginMap();
        if (pluginMap.contains(header.pluginID)) {
            const Plugin::Plugin* pluginContext = &pluginMap.at(header.pluginID);
            if (pluginContext->plugin->onMessage) {
                std::println("Dispatching message to {}", pluginContext->plugin->descriptor->base.type_name);
                pluginContext->plugin->onMessage(pluginContext->plugin, (ExtenDeckSDK::Client*)this, header.messageID, buf.c_str(), buf.length());
            }
        } else {
            std::println("Message recipient not found; discarding...");
        }
        dxstore.lock.unlock();
    } while (dxstore.get_status() == statuses::Running && connection.res > 0 && connection.send_res > 0);
}

bool Client::isConnected() const {
    return connection.isConnected();
}

uint64_t Client::get_uuid() const {
    return m_uuid;
}

void Client::set_nickname(const string& nick) {
    lock.lock();
    this->nickname = nick;
    lock.unlock();
}
const string& Client::get_nickname() const {
    return this->nickname;
}

Profile& Client::getCurrentProfile() {
    return profiles.at(current_profile);
}

json& Client::get_config() {
    return this->config;
}

void Client::create_config() {
    if (config.is_discarded())
        config = json();
    config["profiles"] = json::array();
    json& profileschema = config["profiles"];
    profileschema[0]["uuid"] = generate_uuid();
    profileschema[0] = *Profile::initConfig(&profileschema[0]);
    fs::path cfg_file(get_config_dir() / (std::to_string(m_uuid)+".json"));
    std::ofstream writer(cfg_file);
    writer << config.dump(4);
    writer.close();
    profiles.reserve(1);
}

uint64_t construct_header(uint32_t msg_len, uint32_t type) {
    return (uint64_t)htonl(msg_len) << 32 | htonl(type);
}

void Client::write_config() const {
    fs::path dkd_dir = get_config_dir();
    fs::create_directories(dkd_dir);
    std::ofstream writer(dkd_dir / (std::to_string(m_uuid) + ".json"));
    writer << config.dump(4);
    writer.close();
}

int Client::send(const ExtenDeckSDK::Message& msg, ExtenDeckSDK::PluginBase& sender) {
    return connection.send(msg, &sender);
}

void Client::update_config() {
    send({config.dump(), "com.ExtenDeck.message.Config"}, *Deckastore::get().applicationPlugin->plugin);
}

void Client::draw_properties() {
    Deckastore& dxstore = Deckastore::get();
    Profile& dxprofile = this->getCurrentProfile();
    if (!ImGui::IsPopupOpen("Client properties"))
        ImGui::OpenPopup("Client properties");
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480, 360));
    if (ImGui::BeginPopupModal("Client properties", nullptr, GUI::Constants::modalFlags)) {
        ImGui::TextDisabled("Client ID: %llu", this->m_uuid);
        ImGui::Text("Nickname");
        ImGui::SameLine();
        ImGui::InputText("##nickname", &this->m_nickname);
        //dxprofile.draw_properties(); // TODO: VIEWS
        ImGui::SetCursorPos(ImVec2(340, 326));
        if (ImGui::Button("Cancel", ImVec2(64, 26))) {
            this->m_nickname = nickname;
            dxprofile.properties_cancel();
            ImGui::CloseCurrentPopup();
            dxstore.clientPropertiesShown = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Confirm", ImVec2(64, 26))) {
            this->nickname = m_nickname;
            this->config["nickname"] = m_nickname;

            //dxprofile.properties_apply();

            this->update_config();

            ImGui::CloseCurrentPopup();
            dxstore.clientPropertiesShown = false;
        }

        ImGui::EndPopup();
    }
}

Client::~Client() {
    // this is allowed to fail
    connection.send({"", "org.ExtenDeck.messages.ClientDisconnected"}, Deckastore::get().applicationPlugin->plugin);
    write_config();
    if (nickname.empty())
        std::println("{} disconnected.\n", get_uuid());
    else
        std::println("{} disconnected.\n", get_nickname());
}
}
