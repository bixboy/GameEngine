#include "Gui/Widgets/Layout/SectionContainer.h"
#include "Gui/Core/GuiTheme.h"
#include "imgui.h"


namespace BixEngine::Gui::Widgets::Layout
{
    SectionContainer::SectionContainer(const char* id) : idScope_(id)
        , background_(ImGuiCol_ChildBg, Theme::SectionBackground)
        , rounding_(ImGuiStyleVar_ChildRounding, 5.0f)
        , padding_(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 6.0f))
    {
        ImGuiChildFlags childFlags = ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border;
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar;

        isVisible_ = ImGui::BeginChild(
            "section",
            ImVec2(0.0f, 0.0f),
            childFlags,
            windowFlags
        );
    }

    SectionContainer::~SectionContainer()
    {
        ImGui::EndChild();
        
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
    }
}