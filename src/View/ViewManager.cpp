#include "View/ViewManager.hpp"
#include "SDK/SDK.h"

#include <imgui/imgui.h>

namespace ExtenDeck::View {
void ViewManager::addView(ExtenDeckSDK::ViewBase*(*createView)(ExtenDeckSDK::PluginBase* plugin, ExtenDeckSDK::Profile*, uint64_t), ExtenDeckSDK::PluginBase* parentPlugin, Profile& parentProfile,
                          void(*deleter)(ExtenDeckSDK::ViewBase*), ImVec2 startPos, ImVec2 size, uint64_t uuid) {
    if (!deleter)
        return;
    if (createView)
        viewContainers.emplace_back(
            std::unique_ptr<ExtenDeckSDK::ViewBase, void(*)(ExtenDeckSDK::ViewBase*)>(
                createView(parentPlugin, reinterpret_cast<ExtenDeckSDK::Profile*>(&parentProfile), uuid), deleter
            ), startPos, size
        );
}
void ViewManager::drawViews() const {
    int i = 0;
    for (auto& container : viewContainers) {
        ImVec2 size = container.size == ImVec2(0, 0) ? ImGui::GetWindowSize() : container.size;
        ImVec2 startPos = container.size == ImVec2(0, 0) && container.startPos == ImVec2(0,0) ? currentPos : container.startPos;
        ImGui::SetCursorPos(container.startPos+currentPos);
        ImGui::PushID(i++);
        ImGui::BeginChild("##", size);
        ImVec2 windowSize = ImGui::GetWindowSize();
        container.view->draw(container.view.get());
        ImGui::EndChild();
        ImGui::PopID();
    }
}
}
