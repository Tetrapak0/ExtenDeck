#pragma once

#include "ItemTypeRegistry.hpp"
#include "ShortcutViewPlugin.hpp"
#include "SDK/Message.hpp"
#include "ShortcutView/ShortcutView.hpp"
#include "ShortcutView/Item/ExecutableItem.hpp"
#include "ShortcutView/Item/Item.hpp"

#include <fstream>

namespace ShortcutView::Message {
using ExtenDeck::Networking::MessageBase;

struct MExecute : MessageBase {
    const char* getTypeName() const override {
        return "ShortcutView.Execute";
    }
    void dispatch(ShortcutView& view) {
        uint8_t idx = msg[0];
        if (idx >= view.root->items.size())
            return;
        shared_ptr item = view.root->items[idx];
        if (view.parentPlugin.api.mode == ExtenDeck::modes::Client) {
            if (item->openable_in_editor())
                item->execute();
        } else {
            item->execute();
        }
    }

    MExecute(const string& msg) : MessageBase(msg.c_str(), msg.length()) {}
};
struct MConfig : MessageBase {
    const char* getTypeName() const override {
        return "ShortcutView.Config";
    }
    void dispatch(ShortcutView& view) {
        json cfg = json::parse(msg, nullptr, false, true);
        if (cfg.is_discarded())
            return;

        view.configure(cfg);
    }

    MConfig(const string& msg) : MessageBase(msg.c_str(), msg.length()) {}
};

struct MThumbnailDelivery final : MessageBase {
    const char* getTypeName() const override {
        return "ShortcutView.ThumbnailDelivery";
    }
    void dispatch(ShortcutView& view) {
        view.handleThumbnailDelivery(msg);
    }

    MThumbnailDelivery(const string& msg) : MessageBase(msg.c_str(), msg.length()) {}
};

struct MThumbnailRequest : MessageBase {
    const char* getTypeName() const override {
        return "ShortcutView.ThumbnailRequest";
    }
    void dispatch(ShortcutView& view) {
        uint64_t item_uuid = 0;
        memcpy(&item_uuid, msg, sizeof(uint64_t));
        printf("Received thumbnail request from %llu for %llu\n", view.parentProfile.parent.get_uuid(), item_uuid);
        string path = (view.parentProfile.get_profile_dir() / (std::to_string(item_uuid) + ".png")).string();

        vector<uint8_t> data;

        if (fs::exists(path)) {
            std::ifstream reader(path, std::ios::binary | std::ios::ate);

            std::streamsize len = reader.tellg();
            reader.seekg(0, std::ios::beg);

            data.resize(len);

            reader.read(reinterpret_cast<char*>(data.data()), len);
            reader.close();
        } else {
            printf("File not found, discarding.\n");
            data.resize(3);
            memcpy(data.data(), "NUL", 3);
        }

        string msg;
        msg.append(this->msg);
        msg.append(reinterpret_cast<const char*>(data.data()), data.size());

        view.parentProfile.parent.send(MThumbnailDelivery(msg), view.parentPlugin);
    }

    MThumbnailRequest(const string& msg) : MessageBase(msg.c_str(), msg.length()) {}
};
}
