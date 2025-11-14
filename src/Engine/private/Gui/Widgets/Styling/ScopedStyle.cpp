#include "Gui/Widgets/Styling/ScopedStyle.h"

namespace BixEngine::Gui::Widgets
{
    ScopedStyle::ScopedStyle(ImGuiStyleVar variable, float value) noexcept
    {
        ImGui::PushStyleVar(variable, value);
        engaged_ = true;
    }

    ScopedStyle::ScopedStyle(ImGuiStyleVar variable, const ImVec2& value) noexcept
    {
        ImGui::PushStyleVar(variable, value);
        engaged_ = true;
    }

    ScopedStyle::~ScopedStyle()
    {
        if (engaged_)
            ImGui::PopStyleVar();
    }
}
