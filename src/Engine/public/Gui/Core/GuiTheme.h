#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Theme
{
    extern ImVec4 ContentBackground;
    extern ImVec4 ContentTreeBackground;
    extern ImVec4 HeaderBackground;
    extern ImVec4 BreadcrumbHighlight;
    extern ImVec4 BreadcrumbNormal;
    extern ImVec4 SelectedFolderText;

    extern ImVec4 StatsBackground;
    extern ImVec4 ViewportBackground;
    extern ImVec4 OutlinerBackground;

    extern ImVec4 InspectorBackground;
    extern ImVec4 SectionBackground;
    extern ImVec4 OverviewBackground;

    extern ImVec4 WarningColor;
    extern ImVec4 ErrorColor;

    extern ImVec4 AxisColorX;
    extern ImVec4 AxisColorY;
    extern ImVec4 AxisColorZ;

    inline constexpr float ContentTreeWidth = 240.0f;
    inline constexpr float ThumbnailSize = 72.0f;
    inline constexpr float ThumbnailPadding = 28.0f;

    inline constexpr ImGuiHoveredFlags TooltipHoverFlags = ImGuiHoveredFlags_DelayNormal;
    inline constexpr ImGuiHoveredFlags DoubleClickHoverFlags = ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenOverlapped;
}
