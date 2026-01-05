#include "Gui/Widgets/Builders/SectionBuilder.h"
#include "Gui/Core/GuiTheme.h"


namespace BixEngine::Gui::Widgets
{
    Section::Section(const char* label, bool defaultOpen, ImGuiTreeNodeFlags flags) noexcept
    {
        if (defaultOpen)
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
            
        flags |= ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding; 

        if (ImGui::CollapsingHeader(label, flags))
        {
            Activate();
            ImGui::Indent();
        }
    }

    Section::~Section()
    {
        if (IsActive())
        {
            ImGui::Unindent();
        }
    }
}
