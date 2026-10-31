#include "Client/Client.hpp"
#include "Config/Deckastore.hpp"
#include "GUI/GUI.hpp"
#include "SDK/GUIConstants.hpp"

extern "C" {
    #include "../../resources/icon.h"
}

#include <imgui/imgui_internal.h>

#include <print>
#include <string>
#include <thread>

using std::string;
using std::thread;

namespace ExtenDeck::Server {
void draw_top_bar() {
    Deckastore& dxstore = Deckastore::get();
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(11, 11));
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 48.0f));
    ImGui::Begin("##top_bar", nullptr, GUI::Constants::windowFlags);
    if (ImGui::Button("Settings"))
        dxstore.settingsShown = true;
    ImGui::SameLine();
    ImGui::SetCursorPosX(center.x - 128);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Decks");
    ImGui::SameLine();
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), 11));
    ImGui::SetNextItemWidth(230);
    dxstore.clientManager.mutex.lock();
    const uint64_t& selected_uuid = dxstore.get_selected_uuid();
    const unordered_map<uint64_t, Client::Client>& clients = dxstore.clientManager.getClientsMap();
    const auto selected_client = clients.find(selected_uuid);
    const string label = selected_client == clients.end()
            ? "<none>" : selected_client->second.get_nickname().empty()
            ? std::to_string(selected_uuid) : selected_client->second.get_nickname();
    ImGui::BeginDisabled(clients.empty());
    if (ImGui::BeginCombo("##clients", label.c_str())) {
        for (auto& it : clients) {
            string label = it.second.get_nickname().empty() ? std::to_string(it.first) : it.second.get_nickname();
            const bool selected = selected_uuid == it.first;
            if (ImGui::Selectable(label.c_str(), selected))
                dxstore.set_selected_uuid(it.first);
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Properties"))
        dxstore.clientPropertiesShown = true;
    ImGui::SameLine();
    dxstore.clientManager.mutex.unlock();
    if (ImGui::Button("Disconnect"))
        dxstore.clientManager.request_disconnect(selected_uuid);
    // TODO: (optionally) "lock" client when server's session is locked
    ImGui::EndDisabled();
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void draw_editor() {
    static Deckastore& dxstore = Deckastore::get();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 48.0f));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - 48));
    ImGui::Begin("##ViewEditor", nullptr, GUI::Constants::windowFlags);
    if (!dxstore.clientManager.getNumOfClients()) {
        ImVec2 textSize = ImGui::CalcTextSize("Connect a client to get started");
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x-textSize.x)*0.5f, (ImGui::GetWindowSize().y-textSize.y)*0.5f));
        ImGui::Text("Connect a client to get started");
    } else {
        dxstore.lock.lock();
        Client::Client* client = dxstore.clientManager.retrieve_client(dxstore.get_selected_uuid());
        if (!client) {
            dxstore.lock.unlock();
            ImGui::End();
            return;
        }
        if (!client->getCurrentProfile().viewManager.viewContainers.empty()) {
            client->getCurrentProfile().viewManager.drawViews();
        } else {
            ImVec2 textSize = ImGui::CalcTextSize("+ Add view");
            ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x-textSize.x-ImGui::GetStyle().FramePadding.x)*0.5f, (ImGui::GetWindowSize().y-textSize.y-ImGui::GetStyle().FramePadding.y)*0.5f));
            if (ImGui::Button("+ Add View")) {
                for (auto& [typeID, plugin] : dxstore.pluginManager.getPluginMap()) {
                    if (plugin.plugin->descriptor->type & ExtenDeckSDK::DxPTView) {
                        client->getCurrentProfile().addView(&plugin, ImVec2(0, 0), ImVec2(0, 0)); // Implies fullscreen, centered
                        if (ImGui::Selectable(plugin.plugin->descriptor->base.display_name.data())) {
                        }
                    }
                }
            }
        }
        dxstore.lock.unlock();
    }
    ImGui::End();
}

void server_main_loop(GLFWwindow* window = nullptr, int x = 0, int y = 0) {
    ImGuiIO& io = ImGui::GetIO();
    Deckastore& dxstore = Deckastore::get();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (dxstore.clientPropertiesShown) {
        std::lock_guard lock(dxstore.lock);
        dxstore.retrieve_current_client()->draw_properties();
    } else if (dxstore.settingsShown) {
        dxstore.drawSettings();
    }

    draw_top_bar();
    draw_editor();

#ifdef _DEBUG
    GUI::draw_performance();
#endif

    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

int server_gui_init() {
    Deckastore& dxstore = Deckastore::get();
    const statuses& dxstatus = dxstore.get_status();
    if (dxstore.window.create("ExtenDeck", {1080, 720})) {
        dxstore.set_status(statuses::Exiting);
        return -1;
    }
    DxWindow& dxwindow = dxstore.window;

    GUI::init_context(server_main_loop, server_main_loop);

    std::unique_ptr<Event::EventBus::SubscriptionToken> clientConnectToken;
    clientConnectToken = dxstore.eventBus.subscribeToEvent(
                            dxstore.eventBus.registry.findPrivateDescriptorByName(
                                "org.PoS.ExtenDeck.events.ClientConnected", dxstore.applicationPlugin->plugin), 
                            [](const ExtenDeckSDK::EventBase* event, void* ctx) {
        if (!ctx)
            return;
        Deckastore* dxstore = (Deckastore*)ctx;
        const Event::EClientConnected* ev = (Event::EClientConnected*)event;
        std::println("Inside ClientConnected callback:");
        if (dxstore->get_selected_uuid() == DX_INVALID_UUID)
            dxstore->set_selected_uuid(ev->clientID);
        std::println("Set selected uuid to {}", dxstore->get_selected_uuid());
    }, &dxstore, ExtenDeckSDK::DxECEPImmediate);

    std::unique_ptr<Event::EventBus::SubscriptionToken> clientDisconnectToken;
    clientDisconnectToken = dxstore.eventBus.subscribeToEvent(
                                dxstore.eventBus.registry.findDescriptorByName(
                                    "org.PoS.ExtenDeck.events.ClientDisconnected"),
                                +[](const ExtenDeckSDK::EventBase* event, void* ctx) {
        if (!ctx)
            return;
        Deckastore* dxstore = (Deckastore*)ctx;
        const Event::EClientDisconnected* ev = (Event::EClientDisconnected*)event;
        if (dxstore->get_selected_uuid() == ev->clientID) {
            dxstore->clientPropertiesShown = false;
            if (dxstore->clientManager.getNumOfClients()-1) {
                dxstore->set_selected_uuid(dxstore->clientManager.getClientsMap().begin()->first);
            } else {
                dxstore->set_selected_uuid(DX_INVALID_UUID);
            }
        }
    }, &dxstore, ExtenDeckSDK::DxECEPImmediate);
    std::unique_ptr<Event::EventBus::SubscriptionToken> clientDisconnectIssuedToken;
    clientDisconnectIssuedToken = dxstore.eventBus.subscribeToEvent(
                                    dxstore.eventBus.registry.findDescriptorByName(
                                        "org.PoS.ExtenDeck.events.ClientDisconnectIssued"),
                                    +[](const ExtenDeckSDK::EventBase* event, void* ctx) {
        if (!ctx)
            return;
        Deckastore* dxstore = (Deckastore*)ctx;
        const Event::EClientDisconnectIssued* ev = (Event::EClientDisconnectIssued*)event;
        if (dxstore->get_selected_uuid() == ev->clientID) {
            dxstore->clientPropertiesShown = false;
            if (dxstore->clientManager.getNumOfClients()-1) {
                dxstore->set_selected_uuid(dxstore->clientManager.getClientsMap().begin()->first);
            } else {
                dxstore->set_selected_uuid(DX_INVALID_UUID);
            }
        }
    }, &dxstore, ExtenDeckSDK::DxECEPImmediate); // TODO: Fix;:;:

    Event::ThreadLocalEventBus localEventBus(dxstore.eventBus);
    std::unique_ptr<Event::EventBus::SubscriptionToken> textureReadyToBindToken;
    textureReadyToBindToken = localEventBus.eventBus.subscribeToEvent(
                                    dxstore.eventBus.registry.findPrivateDescriptorByName(
                                        "org.PoS.ExtenDeck.events.TextureReadyToBind", dxstore.applicationPlugin->plugin),
                                    [](const ExtenDeckSDK::EventBase* event, void* ctx) {
        ((Event::ETextureReadyToBind*)event)->texture->bind_to_context();
    }, nullptr, ExtenDeckSDK::DxECEPSubscriberThread);

    while (dxstatus == statuses::Running) {
        localEventBus.processEvents();

        glfwWaitEventsTimeout(GUI::Constants::max_wait_time);
        if (glfwGetWindowAttrib(dxwindow.get(), GLFW_ICONIFIED)) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        if (dxwindow.should_close())
            dxstore.set_status(statuses::Exiting);

        server_main_loop(dxwindow.get());
    };
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    dxstore.window.destroy();
    glfwTerminate();
    return 0;
}
}
