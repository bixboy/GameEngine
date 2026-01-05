#include "Gui/Widgets/Controls/ColorHelpers.h"

#include "Gui/Widgets/Styling/ThemeHelpers.h"


namespace BixEngine::Gui::Widgets::Controls::ColorHelpers
{
    ImVec4 ComputeHoveredColor(const ImVec4& baseColor) noexcept
    {
        return ThemeHelpers::AdjustColor(baseColor, 0.12f);
    }

    ImVec4 ComputeActiveColor(const ImVec4& baseColor) noexcept
    {
        return ThemeHelpers::AdjustColor(baseColor, -0.10f);
    }
}
