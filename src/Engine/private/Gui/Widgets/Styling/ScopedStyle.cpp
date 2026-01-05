#include "Gui/Widgets/Styling/ScopedStyle.h"
#include "imgui.h"


namespace BixEngine::Gui::Widgets::Styling
{
    ScopedStyle::ScopedStyle(ImGuiStyleVar variable, float value) noexcept
    {
        ImGui::PushStyleVar(variable, value);
        active_ = true;
    }

    ScopedStyle::ScopedStyle(ImGuiStyleVar variable, const ImVec2& value) noexcept
    {
        ImGui::PushStyleVar(variable, value);
        active_ = true;
    }

    ScopedStyle::~ScopedStyle()
    {
        if (active_)
        {
            ImGui::PopStyleVar();
        }
    }
}