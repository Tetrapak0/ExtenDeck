#include "Config/State.hpp"
#include "Plugin/PluginManager.hpp"
#include "SDK/SDK.h"

namespace ExtenDeck::Plugin {
PluginManager::PluginManager(Event::EventBus& bus, modes& mode,
                             statuses& status, tasks& running_tasks) : dxDataMembers((uint64_t*)&mode, (uint64_t*)&status, (uint64_t*)&running_tasks) {
    eventAPI.bus = (ExtenDeckSDK::EventBus*)&bus;
}

Plugin* PluginManager::registerPlugin(ExtenDeckSDK::PluginDescriptor* descriptor) {
    if (!descriptor || !descriptor->create || !descriptor->destroy)
        return nullptr;
    uint64_t hash = Utilities::FNV1a_64::hash64(descriptor->base.type_name);
    ExtenDeckSDK::PluginBase* plugin = descriptor->create(&api);
    if (!plugin || !plugin->descriptor)
        return nullptr;
    if (!plugins.contains(hash))
        return &plugins.try_emplace(hash, plugin).first->second;
    return nullptr;
}

Plugin* PluginManager::getPluginByTypename(string type_name) {
    uint64_t hash = Utilities::FNV1a_64::hash64(type_name);
    if (plugins.contains(hash))
        return &plugins.at(hash);
    return nullptr;
}

const unordered_map<uint64_t, Plugin>& PluginManager::getPluginMap() {
    return plugins;
}
}
