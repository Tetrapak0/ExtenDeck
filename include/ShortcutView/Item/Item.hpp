#pragma once

#define DX_ITEMS_MAX 256

#include <string>

#include <jsonhpp/json.hpp>

#include "GUI/Style.hpp"
#include "SDK/SDK.h"

using std::shared_ptr;
using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;
using std::vector;

using json = nlohmann::json;

namespace ShortcutView {
class ShortcutView;

namespace Item {
class FolderItem;

class Item {
public:
    Style style;
    std::string label;
    json* config = nullptr;
    ShortcutView& parentView;
    FolderItem* parentItem = nullptr;

    bool hasTexture = false;
    ExtenDeckSDK::Texture* texture = nullptr;

    // TODO: widgets
    // TODO: Add clock item/widget
    // Vec2<int> u_size; // Size in button units // TODO: Windows 8 style grids

    virtual std::string_view getTypename() {return "";}

    virtual void drawProperties();
    void         drawStyleProperties();
    virtual void propertiesCancel();
    virtual void properties_apply();
    // This function specifies whether the "Apply" button in the properties window
    //      is enabled or disabled. It also specifies whether the button itself is
    //      enabled or disabled.
    virtual bool isDisabled() = 0;
    virtual bool hasBehavioralSettings() {return true;};
    Item*        migrateFrom(Item& other);
    virtual bool isOpenableInEditor();
    uint64_t     getUUID() const;
    int          setUUID(uint64_t uuid);
    int          push_style();
    int          push_colors();
    virtual int  draw(ImVec2 size);
    virtual void execute() = 0;

    Item() = delete;
    virtual ~Item();
protected:
    Style m_style = style;
    std::string m_label = label;
    bool m_hasTexture = hasTexture;
    ExtenDeckSDK::Texture* m_texture = texture;

    explicit Item(json* config, ShortcutView& parentView, FolderItem* parent);
private:
    uint64_t m_uuid = 0;
    bool m_uuid_set = false;
};

// TODO: Implement multi-action buttons
// TODO: Add keypress buttons
}
}
