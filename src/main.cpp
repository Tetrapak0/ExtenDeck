#include "Config/Deckastore.hpp"
#include "Config/Config.hpp"
#include "Client/Client.hpp"
#include "Server/Server.hpp"
#include "Utilities/SingleInstanceSocket.hpp"
#include "SDK/GUIConstants.hpp"

#include <thread>

#ifdef _DEBUG
namespace ExtenDeck {
modes mode_selector(bool& multi_instance) {
    Deckastore& dxstore = Deckastore::get();
    dxstore.window.create("Launch ExtenDeck", {256, 200}, 0, {{GLFW_RESIZABLE, GLFW_FALSE}});
    DxWindow& dxwindow = dxstore.window;

    GUI::init_context();

    ImGuiIO& io = ImGui::GetIO();

    int mode = 0;

    while (true) {
        glfwWaitEventsTimeout(GUI::Constants::max_wait_time);
        if (glfwGetWindowAttrib(dxwindow.get(), GLFW_ICONIFIED)) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        if (dxwindow.should_close()) {
            mode = static_cast<int>(modes::Undefined);
            break;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(256, 200));

        ImGui::Begin("##", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        ImGui::Text("Select launch mode:");
        if (ImGui::RadioButton("Server", !mode)) {
            mode = 0;
        }
        if (ImGui::RadioButton("Client", mode == 1)) {
            mode = 1;
        }
        ImGui::Text("\n");
        ImGui::Checkbox("Allow multiple instances", &multi_instance);

        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 136, ImGui::GetWindowHeight() - 30));
        if (ImGui::Button("Ok", ImVec2(64,26))) {
            ImGui::End();
            break;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(64,26))) {
            mode = static_cast<int>(modes::Undefined);
            ImGui::End();
            break;
        }

        ImGui::End();

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

    return static_cast<modes>(mode);
}
}
#endif

using namespace ExtenDeck;

int main(int argc, char** argv) {
    int ret = 0;
    setbuf(stdout, nullptr);
    setbuf(stderr, nullptr);

#ifdef _WIN32
    if (nx_sock_init()) {
        printf("Failed to initialize networking. WSAGetLastError(): %d\n", WSAGetLastError());
        return -1;
    }
#endif

    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-H"))
            headless = true;
    }
#ifdef _DEBUG
    bool multi_instance = false;
    modes mode = modes::Undefined;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-M"))
            multi_instance = true;
        if (!strcmp(argv[i], "-S") && mode == modes::Undefined)
            mode = modes::Server;
        else if (!strcmp(argv[i], "-C") && mode == modes::Undefined)
            mode = modes::Client;
    }
#endif
    Deckastore& dxstore = Deckastore::get();
    while (dxstore.status != statuses::Exiting) {
        Deckastore::reset();
        if (headless)
            dxstore.headless = true;
#ifdef _DEBUG
        if (mode == modes::Undefined)
            mode = mode_selector(multi_instance);
        if (mode == modes::Undefined) {
            nx_sock_cleanup();
            return 0;
        }
        if (multi_instance)
            Deckastore::get().disable_sis();
#endif
        // TODO: Why isnt this outside the loop
        std::thread t_si_ensurer(si_socket_init);

        // wait for the single instance socket
        while (dxstore.get_status() == statuses::Starting)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (dxstore.check_config()) {
            ret = -1;
            t_si_ensurer.join();
            break;
        }
#ifdef _DEBUG
        Deckastore::get().set_mode(mode);
#endif
        // TODO: maybe move to constructor?
        if (!dxstore.networkManager.query_interfaces()) {
            ret = -1;
            t_si_ensurer.join();
            break;
        }
        switch (dxstore.mode) {
            case modes::Server: {
                ret = Server::run();
                break;
            } case modes::Client: {
                ret = Client::run();
                break;
            } default: {
                ret = -1;
                break;
            }
        }
        t_si_ensurer.join();
    }
    Deckastore::reset();
    nx_sock_cleanup();
    return ret;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow) {
#ifdef _DEBUG
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
#ifdef _MSC_VER
    freopen("CON", "w", stdout);
    freopen("CON", "w", stderr);
    freopen("CON", "r", stdin);
#else
    freopen("CONIN$",  "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONOUT$", "r", stdin);
#endif
#endif
    int ret = main(__argc, __argv);
#ifdef _DEBUG
    FreeConsole();
#endif
    return ret;
}
#endif
