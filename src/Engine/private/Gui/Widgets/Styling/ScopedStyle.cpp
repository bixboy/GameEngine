#include "Gui/Widgets/Styling/ScopedStyle.h"

#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    ScopedStyle::ScopedStyle(ImGuiStyleVar variable, float value) noexcept
    {
        ImGui::PushStyleVar(variable, value);
        Activate();
    }

    ScopedStyle::ScopedStyle(ImGuiStyleVar variable, const ImVec2& value) noexcept
    {
        ImGui::PushStyleVar(variable, value);
        Activate();
    }

    ScopedStyle::~ScopedStyle()
    {
        if (IsActive())
            ImGui::PopStyleVar();
    }
}
