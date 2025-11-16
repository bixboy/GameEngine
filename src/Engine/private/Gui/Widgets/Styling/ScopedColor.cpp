#include "Gui/Widgets/Styling/ScopedColor.h"

#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    ScopedColor::ScopedColor(ImGuiCol colorIndex, const ImVec4& color) noexcept
    {
        ImGui::PushStyleColor(colorIndex, color);
        Activate();
    }

    ScopedColor::~ScopedColor()
    {
        if (IsActive())
            ImGui::PopStyleColor();
    }
}
