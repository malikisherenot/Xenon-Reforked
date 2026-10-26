#pragma once

#include <Geode/Geode.hpp>
#include <imgui-cocos.hpp>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <cstring>

namespace gui
{
    // 1.0 on desktop, scales the whole ImGui UI on phones/tablets
    inline float getUiScale()
    {
#ifdef GEODE_IS_MOBILE
        static float scale = 0.f;
        float h = ImGui::GetIO().DisplaySize.y;
        if (scale <= 0.f && h > 1.f) { scale = std::clamp(h / 540.f, 0.5f, 3.f); }
        return scale > 0.f ? scale : 1.f;
#else
        return 1.f;
#endif
    }

    // drag-to-scroll for the current window (call right before EndChild/End)
    inline void touchScroll(bool hasTitleBar = false)
    {
#ifdef GEODE_IS_MOBILE
        auto& io = ImGui::GetIO();
        if (!ImGui::IsWindowHovered() || ImGui::IsAnyItemActive()) { return; }
        if (hasTitleBar && io.MouseClickedPos[0].y < ImGui::GetWindowPos().y + ImGui::GetFrameHeight() * 1.5f) { return; }
        if (ImGui::IsMouseDragging(0, 8.f * getUiScale())) { ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y); }
#else
        (void)hasTitleBar;
#endif
    }

    // no soft keyboard guarantee on mobile -> auto name when empty
    inline void ensureReplayName(char* name, int size)
    {
#ifdef GEODE_IS_MOBILE
        if (name[0] != '\0') { return; }
        std::time_t t = std::time(nullptr);
        std::tm tmv{};
        localtime_r(&t, &tmv);
        std::strftime(name, size, "macro_%Y%m%d_%H%M%S", &tmv);
#else
        (void)name; (void)size;
#endif
    }
}
