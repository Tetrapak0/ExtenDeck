#include "Client/Client.hpp"
#include "ShortcutView/ItemTypeRegistry.hpp"
#include "ShortcutView/Item/ExecutableItem.hpp"
#include "ShortcutView/Item/FilesystemAndURLItem.hpp"
#include "ShortcutView/Item/FolderItem.hpp"
#include "Config/Profile.hpp"

namespace ShortcutView::Item {
ItemTypeRegistry& ItemTypeRegistry::get() {
    static ItemTypeRegistry itr;
    return itr;
}

const unordered_map<string, ItemTypeRegistry::constructor_t>& ItemTypeRegistry::get_registry() {
    return itemTypeStore;
}

void ItemTypeRegistry::register_type(const char* const item_typename, const constructor_t& constructor) {
    if (!type_exists(item_typename))
        itemTypeStore.emplace(item_typename, constructor);
}

Item* ItemTypeRegistry::createOfType(const char* const item_typename, json* config, ShortcutView& parentView, FolderItem* parent) {
    if (itemTypeStore.find(item_typename) == itemTypeStore.end())
        return nullptr;
    return itemTypeStore[item_typename](config, parentView, parent);
}

bool ItemTypeRegistry::type_exists(const char* const item_typename) const {
    return itemTypeStore.find(item_typename) != itemTypeStore.end();
}

void ItemTypeRegistry::reset() {
    itemTypeStore.clear();
    register_type("Go up", [](json* c, ShortcutView& sv, FolderItem* fi){return new GoUpItem(c, sv, fi);});
    register_type("Folder", [](json* c, ShortcutView& sv, FolderItem* fi){return new FolderItem(c, sv, fi);});
    register_type("Filesystem / URL", [](json* c, ShortcutView& sv, FolderItem* fi){return new FilesystemAndURLItem(c, sv, fi);});
    register_type("Executable", [](json* c, ShortcutView& sv, FolderItem* fi){return new ExecutableItem(c, sv, fi);});
}
}
