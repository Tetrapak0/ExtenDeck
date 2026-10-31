#include "ShortcutView/ShortcutView.hpp"
#include "ShortcutView/ItemTypeRegistry.hpp"
#include "ShortcutView/ShortcutViewPlugin.hpp"
#include "ShortcutView/Item/ExecutableItem.hpp"
#include "SDK/GUIConstants.hpp"

#include "Networking/Networking.hpp"

#include <fstream>
#include <print>

#include "../../include/Plugin/PluginAPI.h"

extern "C" {
    #include "../../resources/EnterIcon.h"
}

namespace fs = std::filesystem;

// TODO: subscribe to file changes and reload on change

using namespace ExtenDeckSDK;

namespace ShortcutView {
static void drawShortcutView(ViewBase* view) {
    ((ShortcutView*)view)->draw();
}
static void drawProperties(ViewBase* view) {
    ((ShortcutView*)view)->drawProperties();
}
static void propertiesApply(ViewBase* view) {
    ((ShortcutView*)view)->propertiesApply();
}
static void propertiesCancel(ViewBase* view) {
    ((ShortcutView*)view)->propertiesCancel();
}
ShortcutView::ShortcutView(PluginBase& parentPlugin, Profile& profile, uint64_t id)
    : ViewBase({"org.PoS.ExtenDeck.plugin.ShortcutView.view.ShortcutView", "ShortcutView"},
               id, &profile, &parentPlugin, drawShortcutView, ::ShortcutView::drawProperties, ::ShortcutView::propertiesApply, ::ShortcutView::propertiesCancel),
        config(prepareConfig(parent_profile->getConfigDir() / (std::to_string(runtimeID) + ".json"))), rows(compute_rows()), columns(compute_columns()),
        items(&config["root"], *this, nullptr) {
    enterIcon = createTextureFromMemory(enter_icon_png, enter_icon_len);
    auto bus = Event::getDefaultEventBus();
    if (getMode() == DxModeClient) {
        serverDisconnectEventToken = bus->subscribeToEvent(bus->findDescriptorByTypename(Event::eventServerDisconnectedTypename.data()),
                                                           [](const EventBase* event, void* ctx) {
            ShortcutView* view = (ShortcutView*)ctx;
            view->root = &view->items;
        }, this, DxECEPImmediate);
    } else {
        clientDisconnectEventToken = bus->subscribeToEvent(bus->findDescriptorByTypename(Event::eventClientDisconnectedTypename.data()),
                                                          [](const EventBase* event, void* ctx) {
            ShortcutView* view = (ShortcutView*)ctx;
            if (((const EClientDisconnected*)event)->clientID == view->parent_profile->getParentClient()->getUUID()) {
                std::unique_lock lock(view->lock);
                view->resetItemPropertiesTab = true;
                view->drawItemProperties();
            }
        }, this, DxECEPImmediate);
        syncConfig();
    }
}

void ShortcutView::writeConfig(json& configOut) {
    std::unique_lock mutex(lock);
    fs::path dkd_dir = parent_profile->getConfigDir();
    fs::create_directories(dkd_dir);
    std::ofstream writer(dkd_dir / (std::to_string(runtimeID) + ".json"));
    writer << configOut.dump(4);
}

void ShortcutView::writeConfig() {
    writeConfig(config);
}

void ShortcutView::reset() {
    rows = compute_rows();
    columns = compute_columns();
    root = &items;
    items.~FolderItem();
    std::ifstream reader(parent_profile->getConfigDir() / (std::to_string(runtimeID) + ".json"));
    config = json::parse(reader);
    reader.close();
    new (&items) Item::FolderItem(&config["root"], *this, nullptr);
}

json ShortcutView::createConfig() {
    return {
        {"rows", 4},
        {"columns", 6},
        {"root", json::object()}
    };
}

json ShortcutView::prepareConfig(fs::path path) {
    json config;
    if (!fs::exists(path)) {
        config = createConfig();

        std::ofstream writer(path);
        writer << config.dump(4);
        return config;
    }
    std::fstream file(path);
    config = json::parse(file, nullptr, false, true);
    if (config.is_discarded()) {
        config = createConfig();
    } else {
        if (!config.contains("rows") || !config["rows"].is_number_unsigned())
            config["rows"] = 4;
        if (!config.contains("columns") || !config["columns"].is_number_unsigned())
            config["columns"] = 6;
        if (!config.contains("root") || !config["root"].is_object())
            config["root"] = json::object();
    }

    file << config.dump(4);

    return config;
}

void ShortcutView::drawServerUI() {
    drawItemProperties();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.2f, 0.2f, 0.2f));
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 button_size = ImVec2(std::floor(windowSize.x / (float)m_columns),
                                std::floor(windowSize.y / (float)m_rows));
    std::unique_lock mutex(lock);
    for (uint8_t i = 0; i < rows*columns; ++i % this->columns ? ImGui::SameLine() : (void)0) {
        ImGui::PushID(i);
        ImVec2 btn_start = ImGui::GetCursorPos();
        shared_ptr<Item::Item> item = root->items[i];
        ImGui::SetNextItemAllowOverlap();
        if (item->draw(button_size))
            draw_item_properties = i;
        if (!item->isDisabled()) {
            if (item->isOpenableInEditor()) {
                ImVec2 pos = ImGui::GetCursorPos();
                ImGui::SetCursorPos(ImVec2(btn_start.x+button_size.x-40, btn_start.y+button_size.y-40));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1,0.1,0.1,1));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3,0.3,0.3,1));
                if (ImGui::ImageButton("##->", enterIcon->getTextureID(), ImVec2(24, 24))) {
                    item->execute();
                    requestExecution(item->getUUID());
                }
                ImGui::PopStyleColor(2);
                ImGui::SetCursorPos(pos);
                ImGui::SetCursorPos(btn_start);
                ImGui::Dummy(button_size);
            }
        }
        ImGui::PopID();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
}

void ShortcutView::drawClientUI() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    ImVec2 button_size = ImVec2(std::floor(ImGui::GetWindowSize().x / (float)columns),
                                std::floor(ImGui::GetWindowSize().y / (float)rows));
    std::unique_lock mutex(lock);
    for (uint8_t i = 0; i < rows*columns;) {
        shared_ptr item = root->items[i];
        ImGui::BeginDisabled(item->isDisabled());
        ImGui::PushID(i);
        if (item->draw(button_size)) {
            requestExecution(item->getUUID());
            if (item->isOpenableInEditor())
                item->execute();
        }

        ImGui::PopID();
        if (++i % columns)
            ImGui::SameLine();
        ImGui::EndDisabled();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

void ShortcutView::drawItemProperties() {
    if (draw_item_properties == -1)
        return;
    Item::ItemTypeRegistry& registry = reinterpret_cast<ShortcutViewPlugin*>(parent_plugin)->getRegistry();
    if (resetItemPropertiesTab) {
        if (ImGui::IsPopupOpen("Button properties"))
            ImGui::CloseCurrentPopup();
        root[draw_item_properties].propertiesCancel();
        draw_item_properties = -1;
        resetItemPropertiesTab = false;
        return;
    }
    static shared_ptr<Item::Item> currentItem = nullptr;
    static shared_ptr<Item::Item> newItem = nullptr;
    if (!ImGui::IsPopupOpen("Button properties")) {
        ImGui::OpenPopup("Button properties");
        currentItem = root->items[draw_item_properties];
        newItem = currentItem;
        resetItemPropertiesTab = true;
    }
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(640, 480));
    if (ImGui::BeginPopupModal("Button properties", nullptr, ExtenDeck::GUI::Constants::modalFlags)) {
        ImGui::BeginChild("##container", ImVec2(ImGui::GetWindowWidth()-ImGui::GetStyle().WindowPadding.x*2, 480-ImGui::GetFontSize()-ImGui::GetStyle().FramePadding.y*2-42-ImGui::GetStyle().WindowPadding.y));
        if (ImGui::BeginTabBar("##categories", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)) {
            if (ImGui::BeginTabItem("General", nullptr, resetItemPropertiesTab ? ImGuiTabItemFlags_SetSelected : 0)) {
                ImGui::BeginChild("##general", ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight()-ImGui::GetFrameHeight()-ImGui::GetStyle().FramePadding.y-1));
                ImGui::BeginDisabled();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Item UUID: %llu", newItem->getUUID());
                ImGui::EndDisabled();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Type: ");
                ImGui::SameLine();
                if (ImGui::BeginCombo("##Type", newItem->getTypename().data())) {
                    for (const auto& [type, constructor] : registry.get_registry()) {
                        const bool selected = newItem->getTypename() == type;
                        ImGui::BeginDisabled(type == "Go up" && !root->parentItem);
                        if (ImGui::Selectable(type.c_str(), selected)) {
                            if (type != newItem->getTypename()) {
                                shared_ptr<Item::Item> swap = newItem;
                                if (currentItem->getTypename() == type)
                                    newItem = currentItem;
                                else
                                    newItem = shared_ptr<Item::Item>(constructor(nullptr, *this, root));
                                newItem->migrateFrom(*swap);
                            }
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                        ImGui::EndDisabled();
                    }
                    ImGui::EndCombo();
                }
                newItem->Item::drawProperties();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::BeginDisabled(!newItem->hasBehavioralSettings());
            if (ImGui::BeginTabItem("Behavior")) {
                ImGui::BeginChild("##behavior", ImVec2(ImGui::GetWindowWidth(),
                                                       ImGui::GetWindowHeight()-ImGui::GetCursorPosY()));
                newItem->drawProperties();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndDisabled();
            if (ImGui::BeginTabItem("Style")) {
                ImGui::BeginChild("##style", ImVec2(ImGui::GetWindowWidth(),
                                                    ImGui::GetWindowHeight() - ImGui::GetCursorPosY()));
                newItem->drawStyleProperties();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
            resetItemPropertiesTab = false;
        }
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(0, ImGui::GetWindowHeight()-34));
        ImGui::BeginChild("##nav", ImVec2(ImGui::GetWindowWidth()-8, 26));
        ImGui::SetCursorPosX(8);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4, 0.0, 0.0, 0.54));
        if (ImGui::Button("Erase", ImVec2(64, 26))) {
            uint64_t uuid = currentItem->getUUID();
            if (currentItem->config) {
                (*currentItem->config)["erase"] = true;
                syncConfig(currentItem.get());
            }
            root->items[draw_item_properties] = std::make_shared<Item::ExecutableItem>(nullptr, *this, root);;
            (*root->config)["items"].erase(uuid);
            ImGui::CloseCurrentPopup();
            draw_item_properties = -1;
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth()-140-60);
        if (ImGui::Button("Clear", ImVec2(64, 26))) {
            auto constructor = registry.get_registry().at(string(newItem->getTypename()));
            std::shared_ptr<Item::Item> swap = newItem;
            newItem = std::shared_ptr<Item::Item>(constructor(swap->config, *this, swap->parentItem));
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(newItem ? newItem->isDisabled() : true);
        if (ImGui::Button("Apply", ImVec2(64, 26))) {
            if (newItem != currentItem)
            root->items[draw_item_properties] = std::make_shared<Item::ExecutableItem>(nullptr, *this, currentItem->parentItem);

            if (!newItem->config)
                newItem->config = &((*newItem->parentItem->config)["items"][std::to_string(newItem->getUUID())] = json::object());
            else if (newItem->getTypename() != currentItem->getTypename())
                newItem->config->clear();

            currentItem = nullptr;

            root->items[draw_item_properties] = newItem;
            item_lookup[newItem->getUUID()] = newItem.get();
            newItem->Item::properties_apply();
            newItem->properties_apply();

            syncConfig(newItem.get());
            newItem = nullptr;

            ImGui::CloseCurrentPopup();
            draw_item_properties = -1;
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(64, 26))) {
            newItem = root->items[draw_item_properties];
            newItem->Item::propertiesCancel();
            newItem->propertiesCancel();
            newItem = nullptr;
            ImGui::CloseCurrentPopup();
            draw_item_properties = -1;
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
}

void ShortcutView::draw() {
    if (getMode() == DxModeClient)
        return drawClientUI();
    return drawServerUI();
}

void ShortcutView::requestExecution(uint64_t uuid) {
    json container{
        {"viewID", runtimeID},
        {"itemID", uuid}
    };
    Message message(container.dump(), mExecTypename);
    parent_profile->getParentClient()->send(message, parent_plugin);
}

void ShortcutView::requestTexture(uint64_t uuid) {
    json request{{"uuid", uuid}};
    Message message{request.dump(), mTextureRequestTypename};
    parent_profile->getParentClient()->send(message, parent_plugin);
}

Item::Item* ShortcutView::getItemByUUID(uint64_t uuid) {
    std::unique_lock mutex(lock);
    if (item_lookup.contains(uuid))
        return item_lookup[uuid];
    return nullptr;
}

void ShortcutView::handleTextureDelivery(string msg) {
    std::unique_lock mutex(lock);
    uint64_t uuid = 0;
    memcpy(&uuid, msg.data(), sizeof(uint64_t));
    uuid = ntohll(uuid);
    fs::path profileDir = parent_profile->getConfigDir();
    fs::create_directories(profileDir);

    msg.erase(msg.begin(), msg.begin() + sizeof(uint64_t));
    if (!memcmp(msg.data(), "NUL", 3))
        return;
    if (!item_lookup.contains(uuid))
        return;

    std::println("Received thumbnail for {}", uuid);

    item_lookup[uuid]->texture = createTextureFromMemory(reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
    fs::path imagePath = fs::path(profileDir) / (std::to_string(uuid) + ".png");
    item_lookup[uuid]->texture->writeTextureAsPNG(imagePath);
}

bool ShortcutView::isUUIDRegistered(uint64_t uuid) {
    return item_lookup.contains(uuid);
}

bool ShortcutView::registerUUID(uint64_t uuid, Item::Item* item) {
    std::unique_lock mutex(lock);
    if (item_lookup.contains(uuid))
        return false;
    item_lookup[uuid] = item;
    return true;
}

void ShortcutView::unregisterUUID(uint64_t uuid) {
    std::unique_lock mutex(lock);
    item_lookup.erase(uuid);
}

uint8_t ShortcutView::compute_rows() const {
    uint64_t t_rows = 4;
    uint64_t t_columns = 6;
    if (config.contains("rows") && config["rows"].is_number_unsigned())
        t_rows = config["rows"];
    if (config.contains("columns") && config["columns"].is_number_unsigned())
        t_columns = config["columns"];
    if (t_rows * t_columns > DX_ITEMS_MAX)
        return 16;
    return t_rows;
}

uint8_t ShortcutView::compute_columns() const {
    uint64_t t_rows = 4;
    uint64_t t_columns = 6;
    if (config.contains("rows") && config["rows"].is_number_unsigned())
        t_rows = config["rows"];
    if (config.contains("columns") && config["columns"].is_number_unsigned())
        t_columns = config["columns"];
    if (t_rows * t_columns > DX_ITEMS_MAX)
        return 16;
    return t_columns;
}

void ShortcutView::drawProperties() {
    std::unique_lock mutex(lock);
    ImGui::Text("Rows");
    ImGui::SameLine();
    static constexpr unsigned char step      = 1;
    static constexpr unsigned char step_fast = 10;
    ImGui::InputScalar("##rows", ImGuiDataType_U8, &m_rows, &step, &step_fast, "%d");
    if (m_rows * m_columns > 256)
        m_rows = rows;
    ImGui::Text("Columns");
    ImGui::SameLine();
    ImGui::InputScalar("##columns", ImGuiDataType_U8, &m_columns, &step, &step_fast, "%d");
    if (m_rows * m_columns > 256)
        m_columns = columns;
}

void ShortcutView::propertiesApply() {
    std::unique_lock mutex(lock);

    root->execute();

    rows = m_rows;
    columns = m_columns;

    config["rows"] = rows;
    config["columns"] = columns;
}

void ShortcutView::propertiesCancel() {
    m_rows = rows;
    m_columns = columns;
}

void ShortcutView::updateConfig(json& recvConfig) {
    if (recvConfig["partial"].get<bool>()) {
        std::unique_lock mutex(lock);

        uint64_t itemID = recvConfig["itemID"];
        uint8_t itemIdx = recvConfig["config"]["idx"];
        Item::FolderItem* parent = (Item::FolderItem*)item_lookup[recvConfig["rootID"]];
        json& itemsConfig = (*parent->config)["items"];
        const json& itemConfig = recvConfig["config"];

        if (recvConfig["config"].contains("erase")) {
            for (size_t i = 0; i < parent->items.size(); ++i) {
                if (parent->items[i]->getUUID() == itemID) {
                    parent->items[i] = std::make_shared<Item::ExecutableItem>(nullptr, *this, parent);
                    item_lookup[parent->items[i]->getUUID()] = parent->items[i].get();

                    item_lookup.erase(itemID);
                    itemsConfig.erase(std::to_string(itemID));
                    break;
                }
            }
        } else {
            // this is to not interfere with the current item's configuration.
            parent->items[itemIdx] = std::make_shared<Item::ExecutableItem>(nullptr, *this, parent);
            itemsConfig[std::to_string(itemID)] = itemConfig;

            parent->items[itemIdx] = std::shared_ptr<Item::Item>(((ShortcutViewPlugin*)parent_plugin)->getRegistry().createOfType(itemConfig["type"].get<string>().c_str(), &itemsConfig[std::to_string(itemID)], *this, parent));
        }
        writeConfig(config);
    } else {
        writeConfig(recvConfig["config"]);
        reset();
    }
}

void ShortcutView::syncConfig() {
    std::unique_lock mutex(lock);
    root = &items;

    writeConfig();

    json configContainer{
        {"viewID", runtimeID},
        {"partial", false},
        {"config", config},
    };

    Message msg{configContainer.dump(), mConfigTypename};
    parent_profile->getParentClient()->send(msg, parent_plugin);
}

void ShortcutView::syncConfig(const Item::Item* item) {
    writeConfig();

    json configContainer{
        {"viewID", runtimeID},
        {"partial", true},
        {"rootID", item->parentItem->getUUID()},
        {"itemID", item->getUUID()},
        {"config", *item->config},
    };

    Message msg{configContainer.dump(), mConfigTypename};
    parent_profile->getParentClient()->send(msg, parent_plugin);
}

ShortcutView::~ShortcutView() {
}
}
