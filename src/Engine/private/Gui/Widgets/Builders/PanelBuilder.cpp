#include "Gui/Widgets/Builders/PanelBuilder.h"

#include "Gui/Widgets/Styling/ScopedColor.h"
#include "Gui/Widgets/Styling/ScopedStyle.h"
#include "imgui.h"

#include <utility>

namespace BixEngine::Gui::Widgets
{
    namespace
    {
        constexpr float kHeaderSpacing = 4.0f;
    }

    void DrawPanelHeader(const PanelHeaderOptions& options)
    {
        if (options.title.empty())
            return;

        ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(kHeaderSpacing, kHeaderSpacing));
        ImGui::TextUnformatted(options.title.c_str());

        if (!options.subtitle.empty())
        {
            ScopedColor disabled(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", options.subtitle.c_str());
        }

        if (options.showSeparator)
            ImGui::Separator();
    }

    PanelBuilder::PanelBuilder(PanelHeaderOptions options)
        : options_(std::move(options))
    {
    }

    void PanelBuilder::DrawHeader() const
    {
        DrawPanelHeader(options_);
    }

    void PanelBuilder::DrawToolbar()
    {
        toolbar_.Commit();
    }
}
