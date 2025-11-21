#include "Gui/Widgets/Layout/SectionContainer.h"

#include "Gui/GuiCommon.h"
#include "imgui.h"

#include <cfloat>

namespace BixEngine::Gui::Widgets
{
    SectionContainer::SectionContainer(const char* id)
        : idScope_(id)
        , background_(ImGuiCol_ChildBg, Theme::SectionBackground)
        , rounding_(ImGuiStyleVar_ChildRounding, 5.0f)
        , padding_(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 6.0f))
    {
        isVisible_ = ImGui::BeginChild(
            "section",
            ImVec2(-FLT_MIN, 0.0f),
            ImGuiChildFlags_AutoResizeY,
            ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar);
    }

    SectionContainer::~SectionContainer()
    {
        ImGui::EndChild();
        ImGui::Dummy(ImVec2(1.0f, 4.0f));
    }
}
