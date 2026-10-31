#pragma once

#include <unordered_map>
#include <functional>

#include "Item/Item.hpp"

using std::string;
using std::unordered_map;

namespace ShortcutView::Item {
class ItemTypeRegistry {
    using constructor_t = std::function<Item*(json*, ShortcutView&, FolderItem*)>;
    unordered_map<string, constructor_t> itemTypeStore;
    void reset();
    // template<typename T>
    // void register_type(const string& name) {
    //     itemtypestore[name] = [](json* cfg, Profile& parent_profile, FolderItem* parent) -> std::unique_ptr<Item> {
    //         return std::make_unique<T>(cfg, parent_profile, parent);
    //     };
    // }
public:
    static ItemTypeRegistry& get();
    const unordered_map<string, constructor_t>& get_registry();
    void register_type(const char* item_typename, const constructor_t& constructor);
    Item* createOfType(const char* item_typename, json* config, ShortcutView& parent_profile, FolderItem* parent);
    bool type_exists(const char* item_typename) const;
    ItemTypeRegistry() {
        this->reset();
    };
};
}
