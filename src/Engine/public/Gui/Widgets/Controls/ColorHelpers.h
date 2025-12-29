#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets::Controls::ColorHelpers
{
     
    ImVec4 ComputeHoveredColor(const ImVec4& baseColor) noexcept;

     
    ImVec4 ComputeActiveColor(const ImVec4& baseColor) noexcept;
}
