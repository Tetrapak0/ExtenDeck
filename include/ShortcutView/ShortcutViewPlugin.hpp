#pragma once

#include "SDK/SDK.h"

#include "ShortcutView/ItemTypeRegistry.hpp"
#include "ShortcutView/ShortcutView.hpp"
#include "Utilities/fnv1a_64.hpp"

namespace ShortcutView {
ExtenDeckSDK::PluginBase* createPlugin(ExtenDeckSDK::DxAPI* api);
void destroyPlugin(ExtenDeckSDK::PluginBase* plugin);
extern ExtenDeckSDK::PluginDescriptor pluginDescriptor;

class ShortcutViewPlugin final : public ExtenDeckSDK::PluginBase {
    Item::ItemTypeRegistry itemTypeRegistry;
    std::unordered_map<uint64_t, ShortcutView*> viewRegistry{};
public:
    Item::ItemTypeRegistry& getRegistry() {
        return itemTypeRegistry;
    }
    static ExtenDeckSDK::ViewBase* createView(PluginBase* parent_plugin, ExtenDeckSDK::Profile* parent_profile, uint64_t id);
    static void destroyView(ExtenDeckSDK::ViewBase* view);
    static void eraseView(ExtenDeckSDK::ViewBase* view);
    static void onMessage(PluginBase* plugin, ExtenDeckSDK::Client* sender, uint64_t messageTypeHash, const char* msg, uint64_t len);
    explicit ShortcutViewPlugin(ExtenDeckSDK::DxAPI* api)
    : PluginBase(&pluginDescriptor, api, nullptr, nullptr,
                    new ExtenDeckSDK::ViewInterface({"org.PoS.ExtenDeck.plugin.ShortcutView.view.ShortcutView",
                                                     "ShortcutView"}, createView, destroyView),
                    onMessage) {
        ExtenDeckSDK::API::init(api);
    }
    ~ShortcutViewPlugin() {
        delete view_interface;
    }
};

constexpr string_view mExecTypename = "com.PoS.ExtenDeck.ShortcutView.messages.Execute";
constexpr string_view mConfigTypename = "com.PoS.ExtenDeck.ShortcutView.messages.Config";
constexpr string_view mTextureRequestTypename = "com.PoS.ExtenDeck.ShortcutView.messages.TextureRequest";
constexpr string_view mTextureDeliveryTypename = "com.PoS.ExtenDeck.ShortcutView.message.TextureDelivery";
enum class MessageTypenameHashes : uint64_t {
    Execute = ExtenDeck::Utilities::FNV1a_64::hash64(mExecTypename),
    Config = ExtenDeck::Utilities::FNV1a_64::hash64(mConfigTypename),
    TextureRequest = ExtenDeck::Utilities::FNV1a_64::hash64(mTextureRequestTypename),
    TextureDelivery = ExtenDeck::Utilities::FNV1a_64::hash64(mTextureDeliveryTypename),
};
}
