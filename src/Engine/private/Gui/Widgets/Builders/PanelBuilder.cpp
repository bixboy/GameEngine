#include "Gui/Widgets/Builders/PanelBuilder.h"

#include "Gui/Widgets/Styling/ScopedColor.h"
#include "Gui/Widgets/Styling/ScopedStyle.h"
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    namespace
    {
        constexpr float kHeaderSpacing = 4.0f;
        constexpr float kContentTopPadding = 4.0f;
    }

    void DrawPanelHeader(const PanelHeaderOptions& options)
    {
        if (options.title.empty())
            return;

        Styling::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(kHeaderSpacing, kHeaderSpacing));
        
        ImGui::TextUnformatted(options.title.c_str());

        if (!options.subtitle.empty())
        {
            Styling::ScopedColor disabled(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", options.subtitle.c_str());
        }

        if (options.showSeparator)
            ImGui::Separator();
    }

    PanelBuilder::PanelBuilder(const PanelHeaderOptions& options, std::function<void(Layout::PanelToolbar&)> toolbarConfig)
    {
        DrawPanelHeader(options);

        if (toolbarConfig)
        {
            toolbarConfig(toolbar_);
            toolbar_.Commit();
        }

        ImGui::Dummy(ImVec2(0.0f, kContentTopPadding));
        
        Activate();
    }

    PanelBuilder::~PanelBuilder()
    {
        if (IsActive())
        {
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            ImGui::End();
        }
    }
}