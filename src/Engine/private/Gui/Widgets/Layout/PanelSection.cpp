#include "Gui/Widgets/Layout/PanelSection.h"
#include "Gui/Widgets/Layout/Spacing.h"
#include "imgui.h"


namespace BixEngine::Gui::Widgets::Layout
{
    PanelSection::PanelSection(const char* label, bool defaultOpen)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (defaultOpen)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;

        open_ = ImGui::CollapsingHeader(label, flags);

        if (open_)
        {
            ImGui::Indent(); 
        }
    }

    PanelSection::~PanelSection()
    {
        if (open_)
        {
            ImGui::Unindent();

            SmallVerticalSpacing(); 
        }
    }
}