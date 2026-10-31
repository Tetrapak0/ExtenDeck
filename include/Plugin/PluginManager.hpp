#pragma once

#include "Config/Config.hpp"
#include "Event/Event.hpp"
#include "Plugin.hpp"
#include "PluginAPI.h"
#include "SDK/SDK.h"

#include <memory>
#include <unordered_map>

using std::shared_ptr;
using std::unordered_map;

namespace ExtenDeck::Plugin {
class PluginManager {
    unordered_map<uint64_t, Plugin> plugins;
public:
    ExtenDeckSDK::ClientAPI clientAPI{
        ExtenDeckSDK::impl::current,
        ExtenDeckSDK::impl::currentProfile,
        ExtenDeckSDK::impl::byUUID,
        ExtenDeckSDK::impl::getUUID,
        ExtenDeckSDK::impl::getNickname,
        ExtenDeckSDK::impl::isConnected,
        ExtenDeckSDK::impl::send,
    };
    ExtenDeckSDK::ProfileAPI profileAPI{
        ExtenDeckSDK::impl::configDir,
        ExtenDeckSDK::impl::getParentClient,
    };
    ExtenDeckSDK::EventAPI eventAPI{
        nullptr,
        ExtenDeckSDK::impl::subscribeToEvent,
        ExtenDeckSDK::impl::postEvent,
        ExtenDeckSDK::impl::unsubscribeFromEvent,
        ExtenDeckSDK::impl::registerEventType,
        ExtenDeckSDK::impl::registerPrivateEventType,
        ExtenDeckSDK::impl::unregisterEventType,
        ExtenDeckSDK::impl::findEventDescriptorByTypename,
        ExtenDeckSDK::impl::findPrivateEventDescriptorByTypename,
        ExtenDeckSDK::impl::createEvent,
        ExtenDeckSDK::impl::retainEvent,
        ExtenDeckSDK::impl::releaseEvent
    };
    ExtenDeckSDK::GraphicsAPI graphicsAPI{
        ExtenDeckSDK::impl::createTextureFromFile,
        ExtenDeckSDK::impl::createTextureFromMemory,
        ExtenDeckSDK::impl::writeTextureAsPNG,
        ExtenDeckSDK::impl::getWidth,
        ExtenDeckSDK::impl::getHeight,
        ExtenDeckSDK::impl::destroyTexture,
        ExtenDeckSDK::impl::overlayTexture,
        ExtenDeckSDK::impl::getTextureID,
        ExtenDeckSDK::impl::isTextureInit,
        ExtenDeckSDK::impl::isTextureReadyToBind,
        ExtenDeckSDK::impl::isTextureNull,
    };
    ExtenDeckSDK::DxFunctions dxfunctions{
        generate_uuid,
        ExtenDeckSDK::impl::freeString,
    };
    ExtenDeckSDK::DxDataMembers dxDataMembers{
        nullptr, nullptr, nullptr
    };
    ExtenDeckSDK::DxAPI api{
        &clientAPI,
        &profileAPI,
        &eventAPI,
        &graphicsAPI,
        &dxfunctions,
        &dxDataMembers
    };
    Plugin* registerPlugin(ExtenDeckSDK::PluginDescriptor* descriptor);
    Plugin* getPluginByTypename(string type_name);
    const unordered_map<uint64_t, Plugin>& getPluginMap();
    PluginManager(Event::EventBus& bus, modes& mode, statuses& status, tasks& running_tasks);
    PluginManager(const PluginManager&) = delete;
    PluginManager operator=(const PluginManager&) = delete;
};
}
