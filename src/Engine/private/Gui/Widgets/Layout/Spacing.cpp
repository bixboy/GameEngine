#include "Gui/Widgets/Layout/Spacing.h"

#include "imgui.h"

namespace BixEngine::Gui::Widgets::Layout
{
    void SmallVerticalSpacing(float height) noexcept
    {
        ImGui::Dummy(ImVec2(1.0f, height));
    }

    void SectionSpacing(float height) noexcept
    {
        ImGui::Dummy(ImVec2(1.0f, height));
    }
}
