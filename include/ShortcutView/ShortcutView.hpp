#pragma once

#include "Item/FolderItem.hpp"
#include "SDK/SDK.h"

#include <filesystem>
#include <mutex>

using std::unordered_map;
namespace fs = std::filesystem;

namespace ShortcutView {
class ShortcutView : public ExtenDeckSDK::ViewBase {
    std::recursive_mutex lock;
    unordered_map<uint64_t, Item::Item*> item_lookup;
public:
    Style style;
    json config = nullptr;

    uint8_t rows = 4;
    uint8_t columns = 6;

    Item::FolderItem* root = &items;
    Item::FolderItem  items;

    void draw();
    void drawItemProperties();
    void drawProperties();
    void propertiesApply();
    void propertiesCancel();
    bool isUUIDRegistered(uint64_t uuid);
    bool registerUUID(uint64_t uuid, Item::Item* item);
    void unregisterUUID(uint64_t uuid);
    void requestTexture(uint64_t uuid);
    Item::Item* getItemByUUID(uint64_t uuid);

    int draw_item_properties = -1;
    bool resetItemPropertiesTab = false;
private:
    uint8_t m_rows = rows;
    uint8_t m_columns = columns;

    ExtenDeckSDK::Texture* enterIcon;

    uint8_t compute_rows()    const;
    uint8_t compute_columns() const;
    
    void requestExecution(uint64_t uuid);
    
    void drawServerUI();
    void drawClientUI();

    static json createConfig();
    static json prepareConfig(fs::path path);
    void syncConfig();
    void syncConfig(const Item::Item* item);
    void writeConfig(json& configOut);

    void writeConfig();
    void reset();

    ExtenDeckSDK::EventBus::upDxSubTok serverDisconnectEventToken;
    ExtenDeckSDK::EventBus::upDxSubTok clientDisconnectEventToken;
public:
    void updateConfig(json& recvConfig);
    void handleTextureDelivery(std::string msg);
    ShortcutView(ExtenDeckSDK::PluginBase& parentPlugin, ExtenDeckSDK::Profile& parent, uint64_t id);
    ~ShortcutView();
};
}
