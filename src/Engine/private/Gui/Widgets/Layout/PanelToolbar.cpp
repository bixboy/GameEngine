#include "Gui/Widgets/Layout/PanelToolbar.h"

#include "Gui/Widgets/Styling/ScopedStyle.h"
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    namespace
    {
        constexpr float kItemSpacingX = 6.0f;
        constexpr float kItemSpacingY = 4.0f;
    }

    void PanelToolbar::AddLeft(const std::function<void()>& drawCallback)
    {
        leftElements_.push_back(drawCallback);
    }

    void PanelToolbar::AddRight(const std::function<void()>& drawCallback)
    {
        rightElements_.push_back(drawCallback);
    }

    void PanelToolbar::Commit()
    {
        if (committed_)
            return;
        committed_ = true;

        ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(kItemSpacingX, kItemSpacingY));

        bool firstInRow = true;
        bool drewAny = false;

        ImGui::BeginGroup();
        for (const auto& draw : leftElements_)
        {
            if (!draw)
                continue;

            if (!firstInRow)
                ImGui::SameLine();

            draw();
            firstInRow = false;
            drewAny = true;
        }
        ImGui::EndGroup();

        for (const auto& draw : rightElements_)
        {
            if (!draw)
                continue;

            if (!firstInRow)
                ImGui::SameLine();

            draw();
            firstInRow = false;
            drewAny = true;
        }

        if (drewAny)
        {
            ImGui::NewLine();
            ImGui::Separator();
        }
    }
}
