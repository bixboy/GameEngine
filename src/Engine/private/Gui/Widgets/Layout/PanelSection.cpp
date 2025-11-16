#include "Gui/Widgets/Layout/PanelSection.h"

#include "Gui/Widgets/Layout/Spacing.h"
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    PanelSection::PanelSection(const char* label, bool defaultOpen, ImGuiTreeNodeFlags flags)
    {
        ImGuiTreeNodeFlags localFlags = flags;
        if (defaultOpen)
            localFlags |= ImGuiTreeNodeFlags_DefaultOpen;
        else
            localFlags &= ~ImGuiTreeNodeFlags_DefaultOpen;

        open_ = ImGui::CollapsingHeader(label, localFlags);
    }

    PanelSection::~PanelSection()
    {
        if (open_)
            Layout::SmallVerticalSpacing();
    }
}
