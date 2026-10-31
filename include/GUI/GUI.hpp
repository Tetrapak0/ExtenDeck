#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <GLFW/include/glfw3.h>

#include <OpenSans/OpenSans.h>

#include <memory>
using std::shared_ptr;

#include "GUI/Texture.hpp"

namespace ExtenDeck::GUI {
void set_colors();
void set_style();
void init_context(GLFWwindowposfun move_callback = nullptr, GLFWwindowsizefun resize_callback = nullptr);
#ifdef _DEBUG
void draw_performance();
#endif

void overlay_texture(const Texture& texture, ImVec2 max_size);
}
