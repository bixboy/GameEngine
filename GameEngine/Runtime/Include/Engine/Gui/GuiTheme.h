#pragma once

#include "imgui.h"

namespace BixEngine::Gui::Theme
{
    extern const ImVec4 ContentBackground;
    extern const ImVec4 ContentTreeBackground;
    extern const ImVec4 HeaderBackground;
    extern const ImVec4 BreadcrumbHighlight;
    extern const ImVec4 BreadcrumbNormal;
    extern const ImVec4 SelectedFolderText;

    extern const ImVec4 StatsBackground;
    extern const ImVec4 ViewportBackground;
    extern const ImVec4 OutlinerBackground;

    extern const ImVec4 InspectorBackground;
    extern const ImVec4 SectionBackground;
    extern const ImVec4 OverviewBackground;

    extern const ImVec4 AxisColorX;
    extern const ImVec4 AxisColorY;
    extern const ImVec4 AxisColorZ;

    inline constexpr float ContentTreeWidth = 240.0f;
    inline constexpr float ThumbnailSize = 72.0f;
    inline constexpr float ThumbnailPadding = 28.0f;

    inline constexpr ImGuiHoveredFlags TooltipHoverFlags = ImGuiHoveredFlags_DelayNormal;
    inline constexpr ImGuiHoveredFlags DoubleClickHoverFlags =
        ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenOverlapped;
}
