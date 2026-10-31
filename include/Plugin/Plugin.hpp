#pragma once

#include "SDK/SDK.h"

#include <string>

#include "ShortcutView/ShortcutViewPlugin.hpp"

using std::string;

namespace ExtenDeck::Plugin {
struct Plugin {
    ExtenDeckSDK::PluginBase* plugin;

    Plugin(ExtenDeckSDK::PluginBase* plugin) : plugin(plugin) {}
    ~Plugin() {
        if (plugin->descriptor->destroy)
            plugin->descriptor->destroy(plugin);
    }
};
}
