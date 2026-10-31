#include "Config/Deckastore.hpp"
#include "Config/Config.hpp"
#include "GUI/GUI.hpp"
#include "GUI/Texture.hpp"
#include "SDK/GUIConstants.hpp"
extern "C" {
    #include "../../resources/icon.h"
}

#include <fstream>
#include <string>

#include <stb/stb_image.h>

using std::ifstream;
using std::ofstream;
using fs::create_directories;
using std::string;

namespace ExtenDeck {
static unsigned int page = 0;
static unsigned int page_bak = page;
static int pageMoveInProgress = 0; // 1 = next, -1 = prev
static int offset = 640;

static int port = 32018;
static modes mode = modes::Server;
static bool discoverable = true;

static bool cancel = false;

static GUI::Texture icon;

static void draw_cancel_dialog() {
    if (!cancel)
        cancel = true;
    static Deckastore& dxstore = Deckastore::get();
    if (!ImGui::IsPopupOpen("Cancel Setup?"))
        ImGui::OpenPopup("Cancel Setup?");
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(280, 160));
    if (ImGui::BeginPopupModal("Cancel Setup?", nullptr, GUI::Constants::modalFlags)) {
        ImGui::TextWrapped("Cancelling setup will terminate ExtenDeck.\n");
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+12);
        ImGui::Text("Cancel Setup?\n");
        ImGui::SetCursorPos(ImVec2(140, 126));
        if (ImGui::Button("Yes", ImVec2(64, 26))) {
            dxstore.set_status(statuses::Exiting);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(64, 26))) {
            glfwSetWindowShouldClose(dxstore.window.get(), GLFW_FALSE);
            cancel = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// TODO: Rewrite this shit PLEASE
static void draw_pages() {
    static Deckastore& dxstore = Deckastore::get();
    ImGuiWindowFlags winflags = GUI::Constants::windowFlags;
    if (pageMoveInProgress == 1) {
        if (offset > 0) offset -= 64;
        else {
            offset = 640;
            pageMoveInProgress = 0;
            page_bak = page;
        }
    } else if (pageMoveInProgress == -1) {
        if (offset < 640) offset += 64;
        else {
            pageMoveInProgress = 0;
            page_bak = page;
        }
    }

    if (!page || (pageMoveInProgress ==  1 && !page_bak) ||
                 (pageMoveInProgress == -1 &&  page_bak == 1)) {
        ImGui::SetNextWindowSize(ImVec2(640, 438));
        ImGui::SetNextWindowPos(ImVec2(offset, 0), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::Begin("#page1", nullptr, winflags);

        ImGui::PushFont(dxstore.headerFont, dxstore.headerFont->LegacySize);
        ImGui::Text("Welcome to ExtenDeck");
        ImGui::PopFont();
        ImGui::Text("Press \"Next\" to begin setting up.");
        if (icon.is_init()) {
            ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth()-136, ImGui::GetWindowHeight()-136));
            ImGui::Image(*icon.get(), ImVec2(128, 128));
        }
        ImGui::End();
    }
    if (page == 1 || (pageMoveInProgress ==  1 && (!page_bak || page_bak == 1)) ||
                     (pageMoveInProgress == -1 && (page_bak == 1 || page_bak == 2))) {
        ImGui::SetNextWindowSize(ImVec2(640, 438));
        if (pageMoveInProgress == 1 && !page_bak)
            ImGui::SetNextWindowPos(ImVec2(offset+640, 0), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        else if (pageMoveInProgress == -1 && page_bak == 1)
            ImGui::SetNextWindowPos(ImVec2(offset, 0));
        else
            ImGui::SetNextWindowPos(ImVec2(offset, 0), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

        ImGui::Begin("#page2", nullptr, winflags);
        ImGui::Text("Should this machine act as a server or client?\n");
        if (ImGui::RadioButton("Server", !static_cast<int>(mode)))
            mode = modes::Server;
        ImGui::SetCursorPosX(24);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Server port: ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(256);
        static constexpr unsigned short step = 1;
        static constexpr unsigned short step_fast = 100;
        ImGui::InputScalar("##port", ImGuiDataType_U16, &port, &step, &step_fast);
        ImGui::BeginDisabled(mode != modes::Server);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+16);
        ImGui::Checkbox("Allow this server to be discovered by clients on your network", &discoverable);
        ImGui::EndDisabled();
        if (ImGui::RadioButton("Client", static_cast<int>(mode)))
            mode = modes::Client;
        ImGui::SetCursorPosY(418);
        ImGui::Text("*You can always change this later.");
        ImGui::End();
    }
    if (page == 2 || ((pageMoveInProgress ==  1 && (page_bak == 1 || page_bak == 2)) ||
                      (pageMoveInProgress == -1 && (page_bak == 2 || page_bak == 3)))) {
        ImGui::SetNextWindowSize(ImVec2(640, 438));
        if (pageMoveInProgress == 1 && page_bak == 1)
            ImGui::SetNextWindowPos(ImVec2(offset+640, 0), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        else if (pageMoveInProgress == -1 && page_bak == 2)
            ImGui::SetNextWindowPos(ImVec2(offset, 0));
        else
            ImGui::SetNextWindowPos(ImVec2(offset, 0), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::Begin("#page3", nullptr, winflags);
        ImGui::PushFont(dxstore.headerFont, dxstore.headerFont->LegacySize);
        ImGui::Text("Setup finished!");
        ImGui::PopFont();
        ImGui::Text("Press \"Finish\" to start using ExtenDeck.");
        if (icon.is_init()) {
            ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth()-136, ImGui::GetWindowHeight()-136));
            ImGui::Image(*icon.get(), ImVec2(128, 128));
        }
        ImGui::End();
    }
}

static void draw_nav() {
    static Deckastore& dxstore = Deckastore::get();
    ImGui::SetNextWindowPos(ImVec2(0, 438));
    ImGui::SetNextWindowSize(ImVec2(640, 42));
    ImGui::Begin("#navbar", nullptr, GUI::Constants::windowFlags);
#ifdef _DEBUG
    static ImGuiIO& io = ImGui::GetIO();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::Text("fps: %.1f; delta %.2f; offset = %d; page, bak = %d, %d čćžđšФシ",
                io.Framerate, io.DeltaTime, offset, page, page_bak);
    ImGui::PopStyleVar();
    ImGui::SameLine();
#endif
    ImGui::SetCursorPosX(432);
    ImGui::BeginDisabled(!page);
    if (ImGui::Button("Back", ImVec2(64, 26))) {
        pageMoveInProgress = -1;
        if (page_bak != page) page_bak = page;
        --page;
        offset = 0;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (page != 2) {
        if (ImGui::Button("Next", ImVec2(64, 26))) {
            pageMoveInProgress = 1;
            if (page_bak != page) page_bak = page;
            ++page;
            offset = 640;
        }
    } else {
        if (ImGui::Button("Finish", ImVec2(64, 26)))
            dxstore.set_status(statuses::Exiting);
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(64, 26))) {
        glfwSetWindowShouldClose(dxstore.window.get(), GLFW_TRUE);
    }
    ImGui::End();
}

void setup_main_loop(GLFWwindow* window = nullptr, int x = 0, int y = 0) {
    ImGuiIO& io = ImGui::GetIO();
    const DxWindow& dxwindow = Deckastore::get().window;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (dxwindow.should_close())
        draw_cancel_dialog();

    draw_pages();
    draw_nav();

    ImGui::Render();
    glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(dxwindow.get());
}

int Deckastore::runSetup() {
    port = server.get_port();
    const statuses& dxstatus = get_status();
    window.create("Set up ExtenDeck", {640, 480}, 0, {std::make_pair(GLFW_RESIZABLE, GLFW_FALSE)});
    if (!window.get()) {
        set_status(statuses::Exiting);
        return -1;
    }

    add_task(tasks::Setup);

    GUI::init_context(setup_main_loop);

    icon.create_from_memory(icon_png, icon_png_len);
    while (!static_cast<int>(dxstatus)) {
        if (icon.ready_to_bind())
            icon.bind_to_context();

        if (pageMoveInProgress)
            glfwPollEvents();
        else
            glfwWaitEventsTimeout(GUI::Constants::max_wait_time);
        if (glfwGetWindowAttrib(window.get(), GLFW_ICONIFIED)) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        setup_main_loop();
    };
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    window.destroy();
    icon.destroy();
    glfwTerminate();
    if (cancel) {
        remove_task(tasks::Setup);
        return -1;
    }
    fs::path dir = get_config_dir();
    create_directories(dir);
    if (fs::exists(dir / "config.json")) {
        ifstream reader(dir/"config.json");
        config = json::parse(reader);
        if (config.is_discarded())
            config = json();
        reader.close();
    }

    config["mode"] = ExtenDeck::mode; // TODO: FIXME:
    config["server"]["discoverable"] = ExtenDeck::discoverable;
    config["server"]["port"] = ExtenDeck::port;
    ofstream writer(dir / "config.json");
    writer << config.dump(4);
    writer.close();

    page = 0;
    page_bak = 0;

    set_status(statuses::Running);
    remove_task(tasks::Setup);
    return 0;
}
}
