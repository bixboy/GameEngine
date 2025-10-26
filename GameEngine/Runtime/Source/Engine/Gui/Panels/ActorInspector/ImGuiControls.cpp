#include "Bix/Engine/Gui/Panels/ActorInspector/ImGuiControls.h"

#include <algorithm>
#include <string>
#include <imgui_internal.h>

namespace BixEngine::Gui::ActorInspector
{
    ImVec4 AdjustColor(const ImVec4& color, float delta)
    {
        const auto clamp = [](float value)
        {
            return std::clamp(value, 0.0f, 1.0f);
        };

        return ImVec4(
            clamp(color.x + delta),
            clamp(color.y + delta),
            clamp(color.z + delta),
            color.w);
    }

    ImVec2 DrawBadge(const char* label, const ImVec4& backgroundColor, const ImVec4& textColor)
    {
        if (!label || label[0] == '\0')
        {
            return ImVec2(0.0f, 0.0f);
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 position = ImGui::GetCursorScreenPos();
        const ImVec2 textSize = ImGui::CalcTextSize(label);
        const ImVec2 padding(6.0f, 3.0f);
        const ImVec2 size(textSize.x + padding.x * 2.0f, textSize.y + padding.y * 2.0f);
        const float rounding = textSize.y * 0.45f;

        drawList->AddRectFilled(position, ImVec2(position.x + size.x, position.y + size.y), ImGui::GetColorU32(backgroundColor), rounding);
        drawList->AddText(ImVec2(position.x + padding.x, position.y + padding.y), ImGui::GetColorU32(textColor), label);

        ImGui::Dummy(size);
        return size;
    }

    bool DrawVector3Control(const char* label, float* values, float resetValue, float speed, const char* format)
    {
        if (!values)
        {
            return false;
        }

        bool changed = false;
        ImGui::PushID(label);
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 120.0f);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label ? label : "");
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));

        const char* axisLabels[3] = {"X", "Y", "Z"};
        const ImVec4 axisColors[3] = {Colors::kAxisXColor, Colors::kAxisYColor, Colors::kAxisZColor};

        for (int index = 0; index < 3; ++index)
        {
            ImGui::PushID(index);

            if (index > 0)
            {
                ImGui::SameLine(0.0f, 4.0f);
            }

            const ImVec4 baseColor = axisColors[index];
            ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, AdjustColor(baseColor, 0.12f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, AdjustColor(baseColor, -0.10f));
            if (ImGui::Button(axisLabels[index], ImVec2(26.0f, 26.0f)))
            {
                values[index] = resetValue;
                changed = true;
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine(0.0f, 4.0f);
            std::string dragId = std::string("##") + axisLabels[index];
            if (ImGui::DragFloat(dragId.c_str(), &values[index], speed, 0.0f, 0.0f, format))
            {
                changed = true;
            }
            ImGui::PopItemWidth();
            ImGui::PopID();
        }

        ImGui::PopStyleVar();
        ImGui::Columns(1);
        ImGui::PopID();
        ImGui::Spacing();

        return changed;
    }

    PersistentSectionScope::PersistentSectionScope(const char* label, const std::string& contextId, bool defaultOpen, ImGuiTreeNodeFlags flags)
        : isOpen_(Utils::BeginPersistentSection(label, contextId, defaultOpen, flags))
    {
    }

    PersistentSectionScope::~PersistentSectionScope()
    {
        if (isOpen_)
        {
            Utils::EndPersistentSection();
        }
    }

    SectionContainer::SectionContainer(const char* id)
    {
        ImGui::PushID(id);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::kSectionBackground);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 6.0f));

        ImGui::BeginChild(
            "section",
            ImVec2(-FLT_MIN, 0.0f),
            ImGuiChildFlags_AutoResizeY,
            ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar);
    }

    SectionContainer::~SectionContainer()
    {
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(1.0f, 4.0f));
        ImGui::PopID();
    }
}

