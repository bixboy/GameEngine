#include "Gui/Widgets/Styling/ThemeHelpers.h"

#include <algorithm>

namespace BixEngine::Gui::Widgets::ThemeHelpers
{
    namespace
    {
        [[nodiscard]] float Clamp01(float value) noexcept
        {
            return std::clamp(value, 0.0f, 1.0f);
        }
    }

    ImVec4 WithAlpha(const ImVec4& color, float alpha) noexcept
    {
        return {color.x, color.y, color.z, Clamp01(alpha)};
    }

    ImVec4 AdjustColor(const ImVec4& color, float delta) noexcept
    {
        return {
            Clamp01(color.x + delta),
            Clamp01(color.y + delta),
            Clamp01(color.z + delta),
            Clamp01(color.w)
        };
    }

    ImU32 ToColor32(const ImVec4& color) noexcept
    {
        return ImGui::GetColorU32(color);
    }
}
