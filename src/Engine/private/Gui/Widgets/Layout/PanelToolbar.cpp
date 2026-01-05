#include "Gui/Widgets/Layout/PanelToolbar.h"
#include "Gui/Widgets/Styling/ScopedStyle.h"
#include "imgui.h"


namespace BixEngine::Gui::Widgets::Layout
{
    namespace
    {
        constexpr float kItemSpacingX = 6.0f;
        constexpr float kItemSpacingY = 4.0f;
    }

    void PanelToolbar::AddLeft(const std::function<void()>& drawCallback)
    {
        if (drawCallback)
            leftElements_.push_back(drawCallback);
    }

    void PanelToolbar::AddRight(const std::function<void()>& drawCallback)
    {
        if (drawCallback)
            rightElements_.push_back(drawCallback);
    }

    void PanelToolbar::Commit()
    {
        if (committed_)
            return;
        
        committed_ = true;

        if (leftElements_.empty() && rightElements_.empty())
            return;

        ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(kItemSpacingX, kItemSpacingY));
        ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

        ImGui::AlignTextToFramePadding();

        // --- GAUCHE ---
        if (!leftElements_.empty())
        {
            ImGui::BeginGroup();
            for (size_t i = 0; i < leftElements_.size(); ++i)
            {
                if (i > 0)
                    ImGui::SameLine();
                
                leftElements_[i]();
            }
            
            ImGui::EndGroup();
        }

        // --- DROITE ---
        if (!rightElements_.empty())
        {
            if (!leftElements_.empty())
            {
                ImGui::SameLine();
            }

            ImGui::BeginGroup();
            for (size_t i = 0; i < rightElements_.size(); ++i)
            {
                if (i > 0)
                    ImGui::SameLine();
                
                rightElements_[i]();
            }
            
            ImGui::EndGroup();
        }

        ImGui::NewLine();
        ImGui::Separator();
    }
}