#include "Gui/Widgets/Controls/DrawVector3Control.h"

#include <imgui_internal.h>
#include <type_traits> // Nécessaire pour std::is_same_v

#include "Gui/Core/GuiCommon.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Utils/MouseWrapping.h"
#include "Gui/Widgets/Controls/ColorHelpers.h"
#include "Gui/Widgets/Styling/ScopedColor.h"
#include "Gui/Widgets/Styling/ScopedStyle.h"
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    namespace
    {
        constexpr float kDefaultColumnWidth = 120.0f;
        constexpr ImVec2 kAxisButtonSize{26.0f, 26.0f};
        constexpr float kItemSpacingX = 4.0f;

        template <typename T>
        bool DrawVector3T(const char* label, T* values, T resetValue, float speed, const char* format)
        {
            if (!values)
                return false;

            bool changed = false;
            const char* idLabel = label ? label : "Vector3Control";
            Utils::ScopedID idScope(idLabel);

            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, kDefaultColumnWidth);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label ? label : "");
            ImGui::NextColumn();

            ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
            ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(kItemSpacingX, 0.0f));

            constexpr const char* axisLabels[3] = {"X", "Y", "Z"};
            const ImVec4 axisColors[3] = {Theme::AxisColorX, Theme::AxisColorY, Theme::AxisColorZ};

            for (int index = 0; index < 3; ++index)
            {
                Utils::ScopedID axisId(index);

                if (index > 0)
                    ImGui::SameLine(0.0f, kItemSpacingX);

                const ImVec4& baseColor = axisColors[index];
                ScopedColor button(ImGuiCol_Button, baseColor);
                ScopedColor hovered(ImGuiCol_ButtonHovered, Controls::ColorHelpers::ComputeHoveredColor(baseColor));
                ScopedColor active(ImGuiCol_ButtonActive, Controls::ColorHelpers::ComputeActiveColor(baseColor));

                if (ImGui::Button(axisLabels[index], kAxisButtonSize))
                {
                    values[index] = resetValue;
                    changed = true;
                }

                ImGui::SameLine(0.0f, kItemSpacingX);

                if constexpr (std::is_same_v<T, float>)
                {
                    const char* fmt = format ? format : "%.3f";
                    if (ImGui::DragFloat("##Value", &values[index], speed, 0.0f, 0.0f, fmt))
                        changed = true;
                }
                else if constexpr (std::is_same_v<T, int>)
                {
                    const char* fmt = format ? format : "%d";
                    if (ImGui::DragInt("##Value", &values[index], speed, 0, 0, fmt))
                        changed = true;
                }

                Utils::ApplyMouseWrapping();
                ImGui::PopItemWidth();
            }

            ImGui::Columns(1);
            ImGui::Spacing();

            return changed;
        }
    }

    // --- DRAW VECTOR3 ---

    // Version Float 
    bool DrawVector3Control(const char* label, float* values, float resetValue, float speed, const char* format)
    {
        return DrawVector3T<float>(label, values, resetValue, speed, format);
    }

    // Version Int 
    bool DrawVector3Control(const char* label, int* values, int resetValue, float speed, const char* format)
    {
        return DrawVector3T<int>(label, values, resetValue, speed, format);
    }
}