#pragma once

#include <string>
#include <vector>

#include <jsonhpp/json.hpp>

#include "GUI/Style.hpp"
#include "Plugin/Plugin.hpp"
#include "View/ViewManager.hpp"

using std::string;
using std::unordered_map;
using std::vector;

namespace fs = std::filesystem;

using json = nlohmann::json;

namespace ExtenDeck::Client {
class Client;
}

namespace ExtenDeck {
class Profile {
    json* config = nullptr;
    string name;
    string m_name = name;
    std::vector<uint64_t> knownViewUUIDs;
public:
    Client::Client& parent;
    View::ViewManager viewManager;
    //Style style;

    void addView(const Plugin::Plugin* plugin, ImVec2 startPos, ImVec2 size);
    static json* initConfig(json* config);
    string compute_name() const;
    const char* getProfileName() const;
    fs::path get_profile_dir() const;
    void draw_properties();
    void properties_apply();
    void properties_cancel();

    explicit Profile(Client::Client& parent, json* config = nullptr);
};
}
