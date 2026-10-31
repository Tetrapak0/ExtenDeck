#include "ShortcutView/Item/Item.hpp"
#include "SDK/SDK.h"
#include "ShortcutView/SHA256.hpp"
#include "ShortcutView/ShortcutView.hpp"

#include <imgui/imgui_stdlib.h>

#ifndef _WIN32
#define NFD_PORTAL
#endif

#include "Utilities/FileDialog.hpp"

#include <filesystem>
#include <fstream>
#include <print>

namespace fs = std::filesystem;

namespace ShortcutView::Item {
Item::Item(json* config, ShortcutView& parentView, FolderItem* parent) : config(config), parentView(parentView), parentItem(parent) {
    if (!parent)
        setUUID(0);
    if (!config) {
        while (!m_uuid_set)
            setUUID(ExtenDeckSDK::generateUUID());
        return;
    }
    if (config->contains("label") && (*config)["label"].is_string()) {
        label = (*config)["label"].get<string>();
        m_label = label;
    }
    if (config->contains("has_texture") && (*config)["has_texture"].is_boolean()) {
        hasTexture = (*config)["has_texture"].get<bool>();
        m_hasTexture = hasTexture;
    }
    if (parent && config->contains("uuid") && (*config)["uuid"].is_number_unsigned())
        setUUID((*config)["uuid"].get<uint64_t>());
    while (!m_uuid_set)
        setUUID(ExtenDeckSDK::generateUUID());
    if (hasTexture) {
        fs::path img_path = parentView.parent_profile->getConfigDir() / std::to_string(parentView.runtimeID) / string(std::to_string(m_uuid) + ".png");
        if (fs::exists(img_path)) {
            this->texture = ExtenDeckSDK::createTextureFromFile(img_path);

            SHA256 hash;
            std::ifstream reader(img_path, std::ios::binary | std::ios::ate);

            std::streamsize len = reader.tellg();
            reader.seekg(0, std::ios::beg);

            vector<uint8_t> data(len);

            reader.read(reinterpret_cast<char*>(data.data()), len);
            reader.close();
            
            hash.update(data.data(), len);

            string hash_str = hash.toString(hash.digest());

            if (config->contains("texture_hash")) {
                if ((*config)["texture_hash"] != hash_str) {
                    std::println("Texture hashes for {} don't match:\n\tJSON: {}\n\tFile: {}\n", this->m_uuid, (*config)["texture_hash"].get<string>().c_str(), hash_str.c_str());
                    if (ExtenDeckSDK::getMode() == ExtenDeckSDK::DxModeClient) {
                        parentView.requestTexture(m_uuid);
                        std::println("Requesting texture for item {}.", m_uuid);
                    } else {
                        (*config)["texture_hash"] = hash_str;
                    }
                }
            } else {
                (*config)["texture_hash"] = hash_str;
            }
        } else if (ExtenDeckSDK::getMode() == ExtenDeckSDK::DxModeClient) {
            std::println("Texture for {} not found... requesting.", m_uuid);
            parentView.requestTexture(m_uuid);
        }
    }
}

void Item::drawStyleProperties() {
    ImGui::Text("Style:");
    for (int i = 0; i < m_style.ImVec2Styles.size(); ++i) {
        Style::StyleVar<ImVec2>& styleVar = m_style.ImVec2Styles[i];
        int tmp[2] = {(int)styleVar.value.x, (int)styleVar.value.y};
        if (ImGui::InputInt2(styleVar.name.data(), tmp)) {
            styleVar.value.x = *tmp;
            styleVar.value.y = tmp[1];
            if (styleVar.value != parentView.style.ImVec2Styles[i].value) {
                m_style.styleOverrideFlags |= styleVar.flag;
            } else {
                m_style.styleOverrideFlags &= ~styleVar.flag;
            }
        }
    }
    for (int i = 0; i < m_style.FloatStyles.size(); ++i) {
        Style::StyleVar<float>& s = m_style.FloatStyles[i];
        if (ImGui::InputFloat(s.name.data(), &s.value)) {
            if (s.value != parentView.style.FloatStyles[i].value)
                m_style.styleOverrideFlags |= s.flag;
            else
                m_style.styleOverrideFlags &= ~s.flag;
        }
    }
    ImGui::Text("Colors:");
    for (int i = 0; i < m_style.colors.size(); ++i) {
        Style::ColVar& c = m_style.colors[i];
        float tmp[4] = {c.value.x, c.value.y, c.value.z, c.value.w};
        if (ImGui::ColorEdit4(c.name.data(), tmp)) {
            c.value.x = *tmp;
            c.value.y = tmp[1];
            c.value.z = tmp[2];
            c.value.w = tmp[3];
            if (c.value != parentView.style.colors[i].value)
                m_style.colorOverrideFlags |= c.flag;
            else
                m_style.colorOverrideFlags &= ~c.flag;
        }
    }
}

void Item::drawProperties() {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Label: ");
    ImGui::SameLine();
    // TODO: Multiline label support
    ImGui::InputText("##label", &m_label);
    ImGui::Checkbox("Use image instead", &m_hasTexture);
    ImGui::SameLine();
    ImGui::BeginDisabled(!m_hasTexture);
    if (ImGui::Button("Browse", ImVec2(64, 26))) {
        string img_path = ExtenDeck::Utilities::nfd_open_image();
        if (!img_path.empty())
            m_texture = ExtenDeckSDK::createTextureFromFile(img_path);
    }
    ImGui::EndDisabled();
    if (m_hasTexture) {
        if (!m_texture && texture)
            m_texture = texture;
        if (m_texture && m_texture->isInit()) {
            float image_aspect = (float)m_texture->getWidth() / (float)m_texture->getWidth();
            float available_aspect = 622.0f / 283.0f;
            ImVec2 size_max;
            if (available_aspect > image_aspect) {
                size_max.y = 283.0f;
                size_max.x = image_aspect * 283.0f;
            } else {
                size_max.x = 622.0f;
                size_max.y = 622.0f / image_aspect;
            }
            ImGui::Image(m_texture->getTextureID(), ImVec2(size_max.x, size_max.y));
        }
    }
}

void Item::propertiesCancel() {
    m_label = label;
    m_hasTexture = hasTexture;
    m_texture = nullptr;
    m_style = style;
}

void Item::properties_apply() {
    int idx = parentView.draw_item_properties;
    label = m_label;
    hasTexture = m_hasTexture;
    fs::path profile_path = parentView.parent_profile->getConfigDir();
    fs::path img_path = profile_path / std::to_string(parentView.runtimeID) / (std::to_string(getUUID()) + ".png");
    if (!hasTexture) {
        if (texture) {
            texture->destroy();
            texture = nullptr;
        }
        if (m_texture) {
            m_texture->destroy();
            m_texture = nullptr;
        }
        config->erase("texture_hash");
        fs::remove(img_path);
    } else if (m_texture) {
        if (m_texture != texture && texture)
            texture->destroy();
        texture = m_texture;
        fs::create_directories(profile_path / std::to_string(parentView.runtimeID));
        texture->writeTextureAsPNG(img_path);
        if (fs::exists(img_path)) {
            SHA256 hash;
            std::ifstream reader(img_path, std::ios::binary | std::ios::ate);

            std::streamsize len = reader.tellg();
            reader.seekg(0, std::ios::beg);

            vector<uint8_t> data(len);

            reader.read(reinterpret_cast<char*>(data.data()), len);
            reader.close();

            hash.update(data.data(), len);

            // TODO: This becomes the actual filename
            (*config)["texture_hash"] = SHA256::toString(hash.digest());
        } else {
            texture->destroy();
            texture = nullptr;
            m_texture = nullptr;
            config->erase("texture_hash");
            hasTexture = false;
            m_hasTexture = false;
        }
    }
    style = m_style;

    (*config)["idx"] = idx;
    (*config)["type"] = parentItem->items[idx]->getTypename();
    (*config)["has_texture"] = hasTexture;
    (*config)["label"] = label;
    (*config)["uuid"] = m_uuid;
}

Item* Item::migrateFrom(Item& other) {
    if (this != &other) {
        m_label = other.m_label;
        parentItem = other.parentItem;
        m_hasTexture = other.m_hasTexture;
        m_texture = std::move(other.m_texture);
        if (m_uuid != other.m_uuid)
            parentView.unregisterUUID(m_uuid);
        m_uuid = other.m_uuid;
    }
    return this;
}

bool Item::isOpenableInEditor() {
    return false;
}

uint64_t Item::getUUID() const {
    return m_uuid;
}

int Item::setUUID(uint64_t uuid) {
    assert(!m_uuid_set);
    if (!parentView.registerUUID(uuid, this))
        return -1;
    m_uuid = uuid;
    m_uuid_set = true;
    if (config)
        (*config)["uuid"] = m_uuid;
    return 0;
}

int Item::push_style() {
    int pushed = 0;
    for (auto& FloatStyle : style.FloatStyles) {
        if (style.styleOverrideFlags & FloatStyle.flag) {
            ImGui::PushStyleVar(FloatStyle.imgui_value, FloatStyle.value);
            ++pushed;
        }
    }
    for (auto& ImVec2Style : style.ImVec2Styles) {
        if (style.styleOverrideFlags & ImVec2Style.flag) {
            ImGui::PushStyleVar(ImVec2Style.imgui_value, ImVec2Style.value);
            ++pushed;
        }
    }
    return pushed;
}

int Item::push_colors() {
    int pushed = 0;
    for (auto & color : style.colors) {
        if (style.colorOverrideFlags & color.flag) {
            ImGui::PushStyleColor(color.imgui_value, color.value);
            ++pushed;
        }
    }
    return pushed;
}

int Item::draw(ImVec2 size) {
    int pushed_styles = push_style();
    int pushed_colors = push_colors();
    if (isDisabled())
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    bool pressed = false;
    if (hasTexture && texture && texture->isInit()) {
        pressed = ImGui::Button("##button", size);
        ExtenDeckSDK::overlayTexture(texture, size.x, size.y);
    } else {
        pressed = ImGui::Button(this->m_label.c_str(), size);
    }
    if (isDisabled())
        ImGui::PopStyleColor();
    ImGui::PopStyleColor(pushed_colors);
    ImGui::PopStyleVar(pushed_styles);
    return pressed;
}

Item::~Item() {
    parentView.unregisterUUID(m_uuid);
    if (!hasTexture)
        return;
    if (texture)
        texture->destroy();
    if (m_texture && m_texture != texture)
        m_texture->destroy();
}
}
