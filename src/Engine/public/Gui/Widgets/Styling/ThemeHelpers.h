#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets::ThemeHelpers
{
     
    ImVec4 WithAlpha(const ImVec4& color, float alpha) noexcept;

     
    ImVec4 AdjustColor(const ImVec4& color, float delta) noexcept;

     
    ImU32 ToColor32(const ImVec4& color) noexcept;
}
