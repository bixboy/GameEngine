#include "Gui/Widgets/Styling/ScopedColor.h"

namespace BixEngine::Gui::Widgets
{
    ScopedColor::ScopedColor(ImGuiCol colorIndex, const ImVec4& color) noexcept
    {
        ImGui::PushStyleColor(colorIndex, color);
        engaged_ = true;
    }

    ScopedColor::~ScopedColor()
    {
        if (engaged_)
            ImGui::PopStyleColor();
    }
}
