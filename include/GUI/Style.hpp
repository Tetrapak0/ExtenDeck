#pragma once

#include <imgui/imgui.h>
#include <array>
#include <string_view>

using std::array;
using std::string_view;

enum StyleVarFlags {
    StyleVar_None               = 0,
    StyleVar_Alpha              = 1,
    StyleVar_DisabledAlpha      = 1<<1,
    StyleVar_WindowPadding      = 1<<2,
    StyleVar_FramePadding       = 1<<3,
    StyleVar_FrameRounding      = 1<<4,
    StyleVar_FrameBorderSize    = 1<<5,
    StyleVar_ItemSpacing        = 1<<6,
    StyleVar_ItemInnerSpacing   = 1<<7,
    StyleVar_GrabMinSize        = 1<<8,
    StyleVar_GrabRounding       = 1<<9
};

enum StyleColFlags {
    StyleCol_None               = 0,
    StyleCol_Text               = 1,
    StyleCol_TextDisabled       = 1<<1,
    StyleCol_WindowBg           = 1<<2,
    StyleCol_Border             = 1<<3,
    StyleCol_FrameBg            = 1<<4,
    StyleCol_Button             = 1<<5,
    StyleCol_ButtonActive       = 1<<6,
    StyleCol_SliderGrab         = 1<<7,
    StyleCol_SliderGrabActive   = 1<<8
};

struct Style {
    template <typename T, typename = std::enable_if_t<std::is_same<T, float>() ||
                                                      std::is_same<T, ImVec2>()>>
    struct StyleVar {
        ImGuiStyleVar_ imgui_value = ImGuiStyleVar_COUNT;
        StyleVarFlags flag = StyleVar_None;
        string_view name;
        T value;
    };

    struct ColVar {
        ImGuiCol_ imgui_value = ImGuiCol_COUNT;
        StyleColFlags flag = StyleCol_None;
        string_view name;
        ImVec4 value;
    };

    Style* parent = nullptr;

    int styleOverrideFlags = StyleVar_None;
    int colorOverrideFlags = StyleCol_None;

    array<StyleVar<ImVec2>, 4> ImVec2Styles = {
        StyleVar{ImGuiStyleVar_WindowPadding, StyleVar_WindowPadding, "Window padding", ImVec2(8, 8)},
        StyleVar{ImGuiStyleVar_FrameRounding, StyleVar_FramePadding, "Frame padding", ImVec2(3, 3)},
        StyleVar{ImGuiStyleVar_ItemSpacing, StyleVar_ItemSpacing, "Item spacing", ImVec2(4, 4)},
        StyleVar{ImGuiStyleVar_ItemInnerSpacing, StyleVar_ItemInnerSpacing, "Item inner spacing", ImVec2(4, 4)}
    };
    array<StyleVar<float>, 6> FloatStyles = {
        StyleVar{ImGuiStyleVar_Alpha, StyleVar_Alpha, "Opacity", 1.0f},
        StyleVar{ImGuiStyleVar_DisabledAlpha, StyleVar_DisabledAlpha, "Opacity (disabled)", 0.5f},
        StyleVar{ImGuiStyleVar_FrameRounding, StyleVar_FrameRounding, "Frame rounding", 0.0f},
        StyleVar{ImGuiStyleVar_FrameBorderSize, StyleVar_FrameBorderSize, "Frame border size", 0.0f},
        StyleVar{ImGuiStyleVar_GrabMinSize, StyleVar_GrabMinSize, "Grab size", 10.0f},
        StyleVar{ImGuiStyleVar_GrabRounding, StyleVar_GrabRounding, "Grab rounding", 0.0f}
    };
    array<ColVar, 9> colors = {
        ColVar{ImGuiCol_Text, StyleCol_Text, "Text", ImVec4(1,1,1,1)},
        ColVar{ImGuiCol_TextDisabled, StyleCol_TextDisabled, "Text (disabled)", ImVec4(0.5f,0.5f,0.5f,1)},
        ColVar{ImGuiCol_WindowBg, StyleCol_WindowBg, "Window background", ImVec4(0,0,0,1)},
        ColVar{ImGuiCol_Border, StyleCol_Border, "Border", ImVec4(0.2f,0.2f,0.2f,0.2f)},
        ColVar{ImGuiCol_FrameBg, StyleCol_FrameBg, "Frame background", ImVec4(0.1f,0.1f,0.1f,0.54f)},
        ColVar{ImGuiCol_Button, StyleCol_Button, "Button", ImVec4(0.1f,0.1f,0.1f,0.54f)},
        ColVar{ImGuiCol_ButtonActive, StyleCol_ButtonActive, "Button (active)", ImVec4(0.2f, 0.2f, 0.2f, 0.54f)},
        ColVar{ImGuiCol_SliderGrab, StyleCol_SliderGrab, "Slider grab", ImVec4(0.34f,0.34f,0.34f,0.54f)},
        ColVar{ImGuiCol_SliderGrabActive, StyleCol_SliderGrabActive, "Slider grab (active)", ImVec4(0.56f,0.56f,0.56f,0.54f)},
    };
};
