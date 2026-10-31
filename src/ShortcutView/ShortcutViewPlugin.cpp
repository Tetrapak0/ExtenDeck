#include <fstream>
#include <print>

#include "SDK/SDK.h"

#include "ShortcutView/ShortcutViewPlugin.hpp"
#include "ShortcutView/ShortcutView.hpp"

#include "Networking/Networking.hpp"

using namespace ExtenDeckSDK;

namespace ShortcutView {
PluginDescriptor pluginDescriptor{{"org.PoS.ExtenDeck.plugin.ShortcutView", "ShortcutView"},
                                  createPlugin, destroyPlugin, DxPTView, DxPFExtensible,
                                  DxPSPAll, DxPRPAll, DX_VERSION_MAJOR, DX_VERSION_MINOR, 0};
PluginBase* createPlugin(DxAPI* api) {
    return new ShortcutViewPlugin(api);
}
void destroyPlugin(PluginBase* plugin) {
    delete (ShortcutViewPlugin*)plugin;
}

ViewBase* ShortcutViewPlugin::createView(PluginBase* parent_plugin, Profile* parent_profile, uint64_t id) {
    return ((ShortcutViewPlugin*)parent_plugin)->viewRegistry[id] = new ShortcutView(*parent_plugin, *parent_profile, id);
}

void ShortcutViewPlugin::destroyView(ViewBase* view) {
    ((ShortcutViewPlugin*)view->parent_plugin)->viewRegistry.erase(view->runtimeID);
    delete (ShortcutView*)view;
}

void ShortcutViewPlugin::onMessage(PluginBase* plugin, Client* sender, uint64_t messageTypeHash, const char* msg, uint64_t len) {
    if (messageTypeHash == (uint64_t)MessageTypenameHashes::TextureDelivery) {
        uint64_t viewID = 0;
        memcpy(&viewID, msg, 8);
        ShortcutView* view = ((ShortcutViewPlugin*)plugin)->viewRegistry.at(ntohll(viewID));
        return view->handleTextureDelivery(string(msg+8, len-8));
    }
    string data(msg, len);
    json container = json::parse(data);
    if (container.is_discarded())
        return;

    ShortcutView* view = ((ShortcutViewPlugin*)plugin)->viewRegistry.at(container["viewID"]);
    if (!view)
        return;
    switch ((MessageTypenameHashes)messageTypeHash) {
        case MessageTypenameHashes::Execute: {
            if (Item::Item* item = view->getItemByUUID(container["itemID"]))
                item->execute();
            break;
        } case MessageTypenameHashes::Config: {
            view->updateConfig(container);
            break;
        } case MessageTypenameHashes::TextureRequest: {
            if (!container.contains("itemID"))
                return;
            uint64_t item_uuid = container["itemID"];
            std::println("Received texture request from {} for {}", sender->getUUID(), item_uuid);
            string path = (view->parent_profile->getConfigDir() / std::to_string(view->runtimeID) / (std::to_string(item_uuid) + ".png")).string();

            vector<uint8_t> imageData;

            if (fs::exists(path)) {
                std::ifstream reader(path, std::ios::binary | std::ios::ate);

                std::streamsize size = reader.tellg();
                reader.seekg(0, std::ios::beg);

                imageData.reserve(size+8);

                reader.read(reinterpret_cast<char*>(imageData.data()+8), size);
                reader.close();
            } else {
                printf("File not found, discarding.\n");
                imageData.reserve(3);
                memcpy(imageData.data(), "NUL", 3);
            }

            item_uuid = htonll(item_uuid);
            memcpy(imageData.data(), &item_uuid, 8);
            Message message{(const char*)imageData.data(), mTextureDeliveryTypename};
            sender->send(message, plugin);
            break;
        }
    }
}
}
