#include "Gui/Widgets/Controls/DrawVector2Control.h"

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
        bool DrawVector2T(const char* label, T* values, T resetValue, float speed, const char* format)
        {
            if (!values)
                return false;

            bool changed = false;
            const char* idLabel = label ? label : "Vector2Control";
            Utils::ScopedID idScope(idLabel);

            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, kDefaultColumnWidth);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label ? label : "");
            ImGui::NextColumn();

            ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
            Styling::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(kItemSpacingX, 0.0f));

            constexpr const char* axisLabels[2] = {"X", "Y"};
            const ImVec4 axisColors[2] = {Theme::AxisColorX, Theme::AxisColorY};

            for (int index = 0; index < 2; ++index)
            {
                Utils::ScopedID axisId(index);

                if (index > 0)
                    ImGui::SameLine(0.0f, kItemSpacingX);

                const ImVec4& baseColor = axisColors[index];
                Styling::ScopedColor button(ImGuiCol_Button, baseColor);
                Styling::ScopedColor hovered(ImGuiCol_ButtonHovered, Controls::ColorHelpers::ComputeHoveredColor(baseColor));
                Styling::ScopedColor active(ImGuiCol_ButtonActive, Controls::ColorHelpers::ComputeActiveColor(baseColor));

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

    // --- DRAW VECTOR 2 ---

    bool DrawVector2Control(const char* label, float* values, float resetValue, float speed, const char* format)
    {
        return DrawVector2T<float>(label, values, resetValue, speed, format);
    }

    bool DrawVector2Control(const char* label, int* values, int resetValue, float speed, const char* format)
    {
        return DrawVector2T<int>(label, values, resetValue, speed, format);
    }
}