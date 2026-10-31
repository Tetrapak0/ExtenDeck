#pragma once

#include <imgui/imgui.h>

namespace ExtenDeck::GUI::Constants {
constexpr unsigned short step = 1;
constexpr unsigned short step_fast = 100;
constexpr double max_wait_time = 1.0f / 60.0f;
constexpr ImGuiWindowFlags modalFlags = ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoCollapse;
constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse |
                                         ImGuiWindowFlags_NoDecoration |
                                         ImGuiWindowFlags_NoMove |
                                         ImGuiWindowFlags_NoResize |
                                         ImGuiWindowFlags_NoBringToFrontOnFocus;
}
