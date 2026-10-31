#include "Config/Profile.hpp"
#include "Client/Client.hpp"
#include "Config/Config.hpp"
#include "Config/Deckastore.hpp"
#include "Utilities/SpaceStripper.hpp"

#include <imgui/imgui_stdlib.h>

namespace ExtenDeck {
Profile::Profile(Client::Client& parent, json* config) : parent(parent), config(initConfig(config)),
                                                         name((*config)["name"]) {
    fs::create_directories(this->get_profile_dir());
    Deckastore& dxstore = Deckastore::get();
    for (auto& view : (*config)["views"]) {
        Plugin::Plugin* plugin = dxstore.pluginManager.getPluginByTypename(view["plugin"]);
        if (!plugin) {
            // TODO: Failed to load message blablabla boring shtuff
            continue;
        }
        viewManager.addView(plugin->plugin->view_interface->createView,
                            plugin->plugin, *this,
                            plugin->plugin->view_interface->destroyView,
                            ImVec2(view["startPos"]["x"].get<int>(), view["startPos"]["y"].get<int>()),
                            ImVec2(view["size"]["w"].get<int>(), view["size"]["h"].get<int>()),
                            view["id"].get<uint64_t>());
    }
}

void Profile::addView(const Plugin::Plugin* plugin, ImVec2 startPos, ImVec2 size) {
    if (!plugin)
        return;
    uint64_t uuid;
    while (std::ranges::find_if(knownViewUUIDs, [&uuid](const uint64_t& u){
        return u == (uuid = generate_uuid());
    }) != knownViewUUIDs.end());

    viewManager.addView(plugin->plugin->view_interface->createView, 
                        plugin->plugin, *this, plugin->plugin->view_interface->destroyView,
                        startPos, size, uuid);

    
}

const char* Profile::getProfileName() const {
    return name.c_str();
}

fs::path Profile::get_profile_dir() const {
    return fs::path(get_config_dir()) / std::to_string(this->parent.get_uuid()) / this->name;
}

json* Profile::initConfig(json* config) {
    if (!config)
        return nullptr;
    if (!config->contains("uuid") || !(*config)["uuid"].is_number_unsigned())
        (*config)["uuid"] = generate_uuid();
    if (!config->contains("name") || !(*config)["name"].is_string()) {
        (*config)["name"] = "Profile" + std::to_string((*config)["uuid"].get<uint64_t>());
        while (fs::exists(get_config_dir() / (*config)["name"])) {
            (*config)["uuid"] = generate_uuid();
            (*config)["name"] = "Profile" + std::to_string((*config)["uuid"].get<uint64_t>());
        }
    }
    if (!config->contains("views") || (*config)["views"].is_array() || (*config)["views"].empty()) {
        (*config)["views"] = json::array();
        (*config)["views"][0] = {
            {"plugin", "org.PoS.ExtenDeck.plugin.ShortcutView"},
            {"id", 0},
            {"startPos", {{"x", 0}, {"y", 0}}},
            {"size", {{"w", 0}, {"h", 0}}}
        };
    }
    for (size_t i = 0; i < (*config)["views"].size(); ++i) {
        // TODO:
    }
    return config;
}

string Profile::compute_name() const {
    if (config->contains("name") && (*config)["name"].is_string()) {
        string stripped = Utilities::strip_spaces((*config)["name"]);
        if (!stripped.empty())
            return stripped;
    }
    uint64_t idx = (*config)["idx"].get<uint64_t>();
    string new_name = "Profile";
    while (fs::exists(new_name + std::to_string(idx)))
        ++idx;
    return new_name + std::to_string(idx);
}

void Profile::draw_properties() {
    ImGui::Text("Profile name");
    ImGui::SameLine();
    ImGui::InputText("##name", &this->m_name);
}

void Profile::properties_apply() {
    name = m_name;
}

void Profile::properties_cancel() {
    m_name = name;
}
}
