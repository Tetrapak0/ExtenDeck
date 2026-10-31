#pragma once

#include <memory>
#include <imgui/imgui.h>

#include "SDK/SDK.h"

namespace ExtenDeck::View {
class ViewContainer {
public:
    std::unique_ptr<ExtenDeckSDK::ViewBase, void(*)(ExtenDeckSDK::ViewBase*)> view;
    ImVec2 size;
    ImVec2 startPos;
    ViewContainer(std::unique_ptr<ExtenDeckSDK::ViewBase, void(*)(ExtenDeckSDK::ViewBase*)> view, ImVec2 size, ImVec2 startPos) : view(std::move(view)), size(size), startPos(startPos) {}
};
}
