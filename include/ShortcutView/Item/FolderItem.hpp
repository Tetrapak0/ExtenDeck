#pragma once

#include "Item.hpp"

namespace ShortcutView::Item {
class FolderItem : public Item {
public:
    vector<shared_ptr<Item>> items;

    static json* checkConfig(json* pConfig);
    std::string_view getTypename() override {return "Folder";}
    bool hasBehavioralSettings() override;
    void drawProperties() override;
    void propertiesCancel() override;
    void properties_apply() override;
    bool isDisabled() override;
    bool isOpenableInEditor() override;
    void execute() override;

    explicit FolderItem(json* config, ShortcutView& parentView, FolderItem* parent);
};

class GoUpItem : public Item {
public:
    std::string_view getTypename() override {return "Go up";}
    bool hasBehavioralSettings() override;
    void drawProperties() override;
    void propertiesCancel() override;
    void properties_apply() override;
    bool isDisabled() override;
    bool isOpenableInEditor() override;
    void execute() override;

    explicit GoUpItem(json* config, ShortcutView& parent_profile, FolderItem* parent);
};
}
