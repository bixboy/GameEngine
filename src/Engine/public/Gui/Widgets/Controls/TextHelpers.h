#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets::Controls::TextHelpers
{
     
    void DrawPropertyLabel(const char* label, float columnWidth) noexcept;

     
    void DrawHelpTooltip(const char* text) noexcept;

     
    ImVec2 ComputeBadgeSize(const char* text, const ImVec2& padding) noexcept;
}
