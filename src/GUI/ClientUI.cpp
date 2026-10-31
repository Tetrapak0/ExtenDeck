#include "Client/Client.hpp"
#include "Client/DiscoveryListenerService.hpp"
#include "Config/Deckastore.hpp"
#include "Event/Event.hpp"
#include "GUI/GUI.hpp"
#include "SDK/GUIConstants.hpp"
#include "SDK/SDK.h"

#include <Tetrapak0/NexusSockets.h>

#include <print>

namespace ExtenDeck::Client {
struct ServerSelectorData {
    Networking::ServerInfo* selected_server_ptr = nullptr;
    int64_t selected_server = -1;
    vector<Networking::ServerInfo> servers;
    mutex serverSelectorLock;
};
void drawConnectDialog(ServerSelectorData& ssd) {
    static bool manual_connect = false;
    static char ip_buffer[16]{};
    static int  port = 32018;
    if (!ImGui::IsPopupOpen("Connect to Server"))
        ImGui::OpenPopup("Connect to Server");
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420, 360));
    // TODO: Auto connect
    if (ImGui::BeginPopupModal("Connect to Server", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        if (manual_connect) {
            // TODO: Add hostname support
            ImGui::Text("IP address: ");
            ImGui::SameLine();
            ImGui::InputText("##ip", ip_buffer, 16);
            ImGui::Text("Port");
            ImGui::SameLine();
            ImGui::InputScalar("##port", ImGuiDataType_U16, &port, &GUI::Constants::step, &GUI::Constants::step_fast);
        } else {
            if (ssd.selected_server >= ssd.servers.size())
                ssd.selected_server = -1;
            if (ssd.servers.empty()) {
                ImVec2 size = ImGui::CalcTextSize("No ExtenDeck servers found on your local network.");
                ImGui::SetCursorPosX((ImGui::GetWindowWidth()-size.x)*0.5f);
                ImGui::TextDisabled("No ExtenDeck servers found on your local network.");
            } else {
                ImVec2 size = ImGui::CalcTextSize("ExtenDeck servers on your local network:");
                ImGui::SetCursorPosX((ImGui::GetWindowWidth()-size.x)*0.5f);
                ImGui::TextDisabled("ExtenDeck servers on your local network:");
                for (int i = 0; i < ssd.servers.size(); ++i) {
                    const bool selected = ssd.selected_server == i;
                    if (ImGui::Selectable(ssd.servers[i].full.c_str(), selected, ImGuiSelectableFlags_NoAutoClosePopups))
                        ssd.selected_server = i;
                }
            }
        }
        ImGui::SetCursorPosY(326);
        ImGui::Checkbox("Enter manually", &manual_connect);
        ImGui::SameLine();
        ImGui::SetCursorPosX(212);
        if (ImGui::Button("Settings", ImVec2(64, 26))) {
            ImGui::CloseCurrentPopup();
            Deckastore::get().settingsShown = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Exit", ImVec2(64, 26))) {
            glfwSetWindowShouldClose(Deckastore::get().window.get(), GLFW_TRUE);
        }
        ImGui::SameLine();
        ImGui::BeginDisabled((!manual_connect && ssd.selected_server == -1) || (manual_connect && !ip_buffer[0]));
        if (ImGui::Button("Connect")) {
            uint16_t porttmp = 0;
            const char* ipv4tmp = nullptr;
            if (manual_connect) {
                ipv4tmp = ip_buffer;
                porttmp = port;
            } else {
                ipv4tmp = ssd.servers[ssd.selected_server].ipv4.c_str();
                porttmp = ssd.servers[ssd.selected_server].port;
            }
            Client* self = Deckastore::get().retrieve_current_client();
            socket_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock != NX_SOCKET_ERROR) {
                sockaddr_in hint{};
                hint.sin_family = AF_INET;
                hint.sin_port = htons(porttmp);
                inet_pton(AF_INET, ipv4tmp, &hint.sin_addr);
                int res = connect(sock, (sockaddr*)&hint, sizeof(hint));
                if (res != NX_SOCKET_ERROR) {
                    uint64_t uuid = htonll(self->get_uuid());
                    // thanks msvc for redundant cast
                    res = send(sock, (const char*)&uuid, sizeof(uint64_t), 0);
                    if (res > 0) {
                        self->connection.~Connection();
                        new (&self->connection) Networking::Connection(sock);
                        if (!manual_connect) {
                            port = porttmp;
                            strncpy(ip_buffer, ipv4tmp, strlen(ipv4tmp));
                            ip_buffer[strlen(ipv4tmp)] = '\0';
                        }
                        ssd.selected_server = -1;
                        ssd.servers.clear();
                        ImGui::CloseCurrentPopup();
                    } else {
                        nx_sock_close(sock);
                    }
                } else {
                    nx_sock_close(sock);
                }
            }
        }
        ImGui::EndDisabled();
        ImGui::EndPopup();
    }
}

static void registerServer(const ExtenDeckSDK::EventBase* event, void* ctx) {
    Event::EServerFound* ev = (Event::EServerFound*)event;
    ServerSelectorData& ssd = *(ServerSelectorData*)ctx;
    std::lock_guard lock(ssd.serverSelectorLock);
    auto it = std::ranges::find_if(ssd.servers, [&ev](const Networking::ServerInfo& si) {
        return si == ev->serverInfo;
    });
    if (it == ssd.servers.end()) {
        std::println("New server found:\n");
        ev->serverInfo.print();
        ssd.servers.emplace_back(ev->serverInfo);
    } else {
        it->last_ping = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    }
}

void pruneOldServers(ServerSelectorData& ssd) {
    std::lock_guard lock(ssd.serverSelectorLock);
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::erase_if(ssd.servers, [now, &ssd](Networking::ServerInfo& server){
        if (now >= server.last_ping+5) {
            std::println("{} timed out.", server.full);
            if (ssd.selected_server_ptr == &server) {
                ssd.selected_server = -1;
                ssd.selected_server_ptr = nullptr;
            }
            return true;
        }
        return false;
    });
}

// TODO: glfwSizeCallback, update server's ViewManager window aspect

int client_gui_init() {
    Deckastore& dxstore = Deckastore::get();
    const statuses& dxstatus = dxstore.get_status();
    Client* dxclient = dxstore.retrieve_current_client();
    const DxWindow& dxwindow = dxstore.window;

    GUI::init_context();

    ImGuiIO& io = ImGui::GetIO();
    //constexpr ImWchar ranges[] = {0x0001, 0xFFFF, 0};
    //ImFont* header = io.Fonts->AddFontFromMemoryCompressedTTF(OpenSans_compressed_data, OpenSans_compressed_size, 32, nullptr, ranges);
    //io.Fonts->Build();

    Event::ThreadLocalEventBus localEventBus(dxstore.eventBus);

    std::unique_ptr<Event::EventBus::SubscriptionToken> serverFoundToken;
    ServerSelectorData ssd;
    while (dxstatus == statuses::Running) {
        localEventBus.processEvents();
        glfwWaitEventsTimeout(GUI::Constants::max_wait_time);
        if (glfwGetWindowAttrib(dxwindow.get(), GLFW_ICONIFIED)) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }
        if (dxwindow.should_close()) {
            dxstore.set_status(statuses::Exiting);
            glfwSetWindowShouldClose(dxwindow.get(), GLFW_FALSE);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // TODO: Font
        //ImGui::PushFont(header);
        if (!dxclient->connection.isConnected()) {
            if (glfwGetInputMode(dxstore.window.get(), GLFW_CURSOR) != GLFW_CURSOR_NORMAL)
                glfwSetInputMode(dxstore.window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

            pruneOldServers(ssd);
            if (!serverFoundToken) {
                Networking::ServerInfo si("", 0);
                std::println("client_gui_init: Subscribing to EServerFound");
                serverFoundToken = dxstore.eventBus.subscribeToEvent(
                                        dxstore.eventBus.registry.findPrivateDescriptorByName(
                                                "org.PoS.ExtenDeck.events.ServerFound", dxstore.applicationPlugin->plugin),
                                        registerServer, &ssd, ExtenDeckSDK::DxECEPWorkerThread);
                std::println("{}", (intptr_t)serverFoundToken.get());
            }

            if (dxstore.settingsShown)
                dxstore.drawSettings();
            else
                drawConnectDialog(ssd);
        } else {
            if (glfwGetInputMode(dxstore.window.get(), GLFW_CURSOR) != GLFW_CURSOR_HIDDEN)
                glfwSetInputMode(dxstore.window.get(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }

        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::Begin("##mainWindow", nullptr, GUI::Constants::windowFlags);
        dxstore.lock.lock();
        dxclient->getCurrentProfile().viewManager.drawViews();
        dxstore.lock.unlock();
        ImGui::End();

        //ImGui::PopFont();
//#ifdef _DEBUG
//        GUI::draw_performance();
//#endif

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(dxwindow.get());
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    dxstore.window.destroy();
    glfwTerminate();
    return 0;
}
}
