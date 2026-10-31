#include "ShortcutView/ItemTypeRegistry.hpp"
#include "ShortcutView/Item/ExecutableItem.hpp"
#include "ShortcutView/Item/FolderItem.hpp"
#include "ShortcutView/ShortcutView.hpp"
#include "ShortcutView/ShortcutViewPlugin.hpp"

namespace ShortcutView::Item {
FolderItem::FolderItem(json* config, ShortcutView& parentView, FolderItem* parent)
: Item(checkConfig(config), parentView, parent) {
    if (!config)
        return;
    items.reserve(DX_ITEMS_MAX);
    items.resize(parentView.rows*parentView.columns);
    ItemTypeRegistry& type_registry = reinterpret_cast<ShortcutViewPlugin*>(parentView.parent_plugin)->getRegistry();
    for (auto& item : items)
        item = std::make_shared<ExecutableItem>(nullptr, parentView, this);
    json& itemsConfig = (*config)["items"];
    for (auto it = itemsConfig.begin(); it != itemsConfig.end();) {
        string uuidStr = it.key();
        json& itemConfig = it.value();
        if (!itemConfig.contains("idx") || !itemConfig["idx"].is_number_integer() ||
            itemConfig["idx"] >= items.size() || itemConfig["idx"] < 0) {
            it = itemsConfig.erase(it);
            continue;
        }
        if (!itemConfig.contains("type") || !itemConfig["type"].is_string()) {
            it = itemsConfig.erase(it);
            continue;
        }
        uint64_t uuid = std::strtoull(uuidStr.c_str(), nullptr, 10);
        if (uuid && !parentView.isUUIDRegistered(uuid)) {
            itemConfig["uuid"] = uuid;
            ++it;
        } else {
            do
                uuid = ExtenDeckSDK::generateUUID();
            while (parentView.isUUIDRegistered(uuid));
            json backupConfig = itemConfig;
            it = itemsConfig.erase(it);
            backupConfig["uuid"] = uuid;
            itemsConfig[std::to_string(uuid)] = backupConfig;
        }
    }
    for (auto& [uuidStr, itemConfig] : itemsConfig.items()) {
        string type = itemConfig["type"].get<string>();
        if (type_registry.type_exists(type.c_str())) {
            items[itemConfig["idx"]] = std::shared_ptr<Item>(type_registry.createOfType(type.c_str(), &itemConfig, parentView, this));
        } else {
            // TODO: Message center: show error, missing plugin, etc. (LATER!!!)
        }
    }
}

json* FolderItem::checkConfig(json* pConfig) {
    if (!pConfig)
        return nullptr;
    if (!pConfig->contains("items") || !(*pConfig)["items"].is_object())
        (*pConfig)["items"] = json::object();
    
    return pConfig;
}

bool FolderItem::hasBehavioralSettings() {
    return false;
}

void FolderItem::drawProperties() {}

void FolderItem::propertiesCancel() {}

void FolderItem::properties_apply() {
    if (items.size() < parentView.rows*parentView.columns) {
        for (size_t i = items.size(); i < parentView.rows*parentView.columns; ++i) {
            items.emplace_back(std::make_shared<ExecutableItem>(nullptr, parentView, this));
        }
    }
    if (!config->contains("items") || !(*config)["items"].is_object())
        (*config)["items"] = json::object();
}

bool FolderItem::isDisabled() {
    return false;
}

bool FolderItem::isOpenableInEditor() {
    return true;
}

void FolderItem::execute() {
    if (items.size() < parentView.rows*parentView.columns) {
        for (size_t i = items.size(); i < parentView.rows*parentView.columns; ++i)
            items.emplace_back(std::make_shared<ExecutableItem>(nullptr, parentView, this));
    } else if (items.size() > parentView.rows*parentView.columns) {
        items.erase(items.begin() + items.size(), items.end());
    }
    parentView.root = this;
}



GoUpItem::GoUpItem(json* config, ShortcutView& parent_profile, FolderItem* parent)
: Item(config, parent_profile, parent) {}

void GoUpItem::drawProperties() {}

bool GoUpItem::hasBehavioralSettings() {
    return false;
}

void GoUpItem::propertiesCancel() {}

void GoUpItem::properties_apply() {}

bool GoUpItem::isDisabled() {
    return parentItem->parentItem ? false : true;
}

bool GoUpItem::isOpenableInEditor() {
    return true;
}

void GoUpItem::execute() {
    if (!isDisabled()) {
        parentItem->parentItem->execute();
    }
}
}
