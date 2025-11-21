#pragma once
#include "imgui.h"
#include <cstdint>

namespace BixEngine::Gui
{
    enum class DockSpaceRegion : std::uint8_t
    {
        Center = 0,
        Left,
        Right,
        Bottom,
        Top,
        Count
    };
}

namespace BixEngine::Gui::Theme
{
    inline const ImVec4 ContentBackground{0.09f, 0.09f, 0.09f, 0.95f};
    inline const ImVec4 ContentTreeBackground{0.13f, 0.13f, 0.13f, 0.95f};
    inline const ImVec4 HeaderBackground{0.16f, 0.16f, 0.16f, 1.0f};
    inline const ImVec4 BreadcrumbHighlight{0.95f, 0.80f, 0.40f, 1.0f};
    inline const ImVec4 BreadcrumbNormal{0.75f, 0.75f, 0.75f, 1.0f};
    inline const ImVec4 SelectedFolderText{0.95f, 0.85f, 0.40f, 1.0f};

    inline const ImVec4 StatsBackground{0.10f, 0.10f, 0.10f, 0.95f};
    inline const ImVec4 ViewportBackground{0.05f, 0.05f, 0.05f, 1.0f};
    inline const ImVec4 OutlinerBackground{0.11f, 0.11f, 0.11f, 0.95f};

    inline const ImVec4 InspectorBackground{0.12f, 0.12f, 0.12f, 0.95f};
    inline const ImVec4 SectionBackground{0.18f, 0.18f, 0.18f, 0.65f};
    inline const ImVec4 OverviewBackground{0.16f, 0.16f, 0.19f, 0.85f};

    inline const ImVec4 AxisColorX{0.80f, 0.28f, 0.28f, 1.0f};
    inline const ImVec4 AxisColorY{0.32f, 0.72f, 0.45f, 1.0f};
    inline const ImVec4 AxisColorZ{0.26f, 0.45f, 0.86f, 1.0f};

    inline constexpr float ContentTreeWidth = 240.0f;
    inline constexpr float ThumbnailSize = 72.0f;
    inline constexpr float ThumbnailPadding = 28.0f;

    inline constexpr ImGuiHoveredFlags TooltipHoverFlags = ImGuiHoveredFlags_DelayNormal;
    inline constexpr ImGuiHoveredFlags DoubleClickHoverFlags =
        ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenOverlapped;
}
