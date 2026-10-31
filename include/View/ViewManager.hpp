#pragma once

#include "ViewContainer.hpp"

#include <vector>

namespace ExtenDeck {
    class Profile;
}

namespace ExtenDeck::View {
class ViewManager {
    ImVec2 currentPos;
public:
    std::vector<ViewContainer> viewContainers;
    void addView(ExtenDeckSDK::ViewBase*(*createView)(ExtenDeckSDK::PluginBase*,
                                                      ExtenDeckSDK::Profile*, uint64_t),
                 ExtenDeckSDK::PluginBase* parentPlugin,
                 Profile& parentProfile,
                 void(*deleter)(ExtenDeckSDK::ViewBase*),
                 ImVec2 startPos, ImVec2 size, uint64_t uuid);
    void drawViews() const;
};
}
