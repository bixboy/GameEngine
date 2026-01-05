#include "Gui/Widgets/Styling/ScopedColor.h"
#include "imgui.h"


namespace BixEngine::Gui::Widgets::Styling
{
    ScopedColor::ScopedColor(ImGuiCol colorIndex, const ImVec4& color) noexcept
    {
        ImGui::PushStyleColor(colorIndex, color);
        active_ = true;
    }

    ScopedColor::ScopedColor(ImGuiCol colorIndex, ImU32 color) noexcept
    {
        ImGui::PushStyleColor(colorIndex, color);
        active_ = true;
    }

    ScopedColor::~ScopedColor()
    {
        if (active_)
        {
            ImGui::PopStyleColor();
        }
    }
}