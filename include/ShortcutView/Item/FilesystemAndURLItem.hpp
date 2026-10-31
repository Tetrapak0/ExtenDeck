#pragma once

#include "Item.hpp"

namespace ShortcutView::Item {
class FilesystemAndURLItem : public Item {
public:
    std::string path;

    explicit FilesystemAndURLItem(json* config, ShortcutView& parentView, FolderItem* parent);

    std::string_view getTypename() override {return "Filesystem / URL";}
    void drawProperties() override;
    void propertiesCancel() override;
    void properties_apply() override;
    bool isDisabled() override;
    void execute() override;
protected:
    std::string m_path = path;
};
}
