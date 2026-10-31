#include "Config/Deckastore.hpp"
#include "Config/Config.hpp"
#include "GUI/GUI.hpp"
#include "SDK/GUIConstants.hpp"

#include <imgui/imgui_freetype.h>
#include "Roboto/RobotoRegular.h"

namespace ExtenDeck::GUI {
void overlay_texture(const Texture& texture, ImVec2 max_size) {
    ImVec2 button_min = ImGui::GetItemRectMin();

    float img_w = static_cast<float>(texture.get_width());
    float img_h = static_cast<float>(texture.get_height());
    float btn_w = max_size.x;
    float btn_h = max_size.y;

    float img_aspect = img_w / img_h;
    float btn_aspect = btn_w / btn_h;

    ImVec2 draw_size;
    if (img_aspect > btn_aspect) {
        draw_size.x = btn_w;
        draw_size.y = btn_w / img_aspect;
    } else {
        draw_size.y = btn_h;
        draw_size.x = btn_h * img_aspect;
    }

    ImVec2 padding((btn_w - draw_size.x) * 0.5f, (btn_h - draw_size.y) * 0.5f);

    ImGui::GetWindowDrawList()->AddImage(texture.getTextureID(),
                                         ImVec2(button_min.x + padding.x,
                                                button_min.y + padding.y),
                                         ImVec2(button_min.x + padding.x + draw_size.x,
                                                button_min.y + padding.y + draw_size.y));
}

void init_context(GLFWwindowposfun move_callback, GLFWwindowsizefun resize_callback) {
    Deckastore& dxstore = Deckastore::get();
    const DxWindow& dxwindow = dxstore.window;
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGui::StyleColorsDark();

    set_colors();
    set_style();

    io.Fonts->FontLoaderFlags = ImGuiFreeTypeLoaderFlags_ForceAutoHint | ImGuiFreeTypeLoaderFlags_LightHinting | ImGuiFreeTypeLoaderFlags_LoadColor;
    constexpr ImWchar ranges[] = {0x0001, 0xFFFF, 0};
    dxstore.regularFont = io.Fonts->AddFontFromMemoryCompressedTTF(Roboto_Regular_compressed_data, Roboto_Regular_compressed_size, 18, nullptr, ranges);
    dxstore.headerFont  = io.Fonts->AddFontFromMemoryCompressedTTF(Roboto_Regular_compressed_data, Roboto_Regular_compressed_size, 36, nullptr, ranges);
    //io.Fonts->AddFontFromFileTTF("NotoSerifCJKjp-Bold.otf", 18, nullptr);

    ImGui_ImplGlfw_InitForOpenGL(dxstore.window.get(), true);
    ImGui_ImplOpenGL3_Init();

    if (move_callback)
        dxwindow.register_move_callback(move_callback);
    if (resize_callback)
        dxwindow.register_resize_callback(resize_callback);
}

#ifdef _DEBUG
bool show_demo_window = false;
void draw_performance() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("Performance metrics", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize);
    ImGui::Checkbox("Demo  ", &show_demo_window);
    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
        ImGui::ShowMetricsWindow(&show_demo_window);
    }

    ImGui::SameLine();
    ImGui::Text("Average %.3f ms/f (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
}
#endif
}
