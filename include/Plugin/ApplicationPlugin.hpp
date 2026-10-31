#pragma once

#include "SDK/SDK.h"

namespace ExtenDeck {
ExtenDeckSDK::PluginBase* createApplicationPlugin(ExtenDeckSDK::DxAPI* api);
void destroyApplicationPlugin(ExtenDeckSDK::PluginBase* plugin);
inline ExtenDeckSDK::PluginDescriptor applicationPluginDescriptor{{"org.PoS.ExtenDeck.application", "ExtenDeck"},
                    createApplicationPlugin, destroyApplicationPlugin, ExtenDeckSDK::DxPTHelper, ExtenDeckSDK::DxPFNone,
                    ExtenDeckSDK::DxPSPAll, ExtenDeckSDK::DxPRPAll, DX_VERSION_MAJOR,
                    DX_VERSION_MINOR, 0};
struct ApplicationPlugin final : ExtenDeckSDK::PluginBase {
    static void onMessage(PluginBase* plugin, ExtenDeckSDK::Client* sender,
                          uint64_t messageTypeHash, const char* msg, uint64_t len) {

    }
    ApplicationPlugin(ExtenDeckSDK::DxAPI& api)
    : PluginBase(&applicationPluginDescriptor, &api, nullptr, nullptr, nullptr, onMessage) {}
};
inline ExtenDeckSDK::PluginBase* createApplicationPlugin(ExtenDeckSDK::DxAPI* api) {
    return new ApplicationPlugin(*api);
}
inline void destroyApplicationPlugin(ExtenDeckSDK::PluginBase* plugin) {
    delete (ApplicationPlugin*)plugin;
}
} // namespace ExtenDeck
