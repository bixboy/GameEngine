#include "Engine/Gui/Panels/ActorInspector/ImGuiControls.h"

#include <algorithm>
#include <string>

#include <imgui_internal.h>

namespace BixEngine::Gui::ActorInspector
{
    using namespace Theme;
    using namespace Utils;

    ImVec2 DrawBadge(const char* label, const ImVec4& backgroundColor, const ImVec4& textColor)
    {
        if (!label || label[0] == '\0')
            return ImVec2(0.0f, 0.0f);

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
            return false;

        bool changed = false;
        const char* idLabel = label ? label : "Vector3Control";
        ScopedID idScope(idLabel);

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 120.0f);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label ? label : "");
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));

        constexpr const char* axisLabels[3] = {"X", "Y", "Z"};
        const ImVec4 axisColors[3] = {AxisColorX, AxisColorY, AxisColorZ};

        for (int index = 0; index < 3; ++index)
        {
            ScopedID axisId(index);

            if (index > 0)
                ImGui::SameLine(0.0f, 4.0f);

            const ImVec4& baseColor = axisColors[index];
            ScopedColor button(ImGuiCol_Button, baseColor);
            ScopedColor hovered(ImGuiCol_ButtonHovered, AdjustColor(baseColor, 0.12f));
            ScopedColor active(ImGuiCol_ButtonActive, AdjustColor(baseColor, -0.10f));

            if (ImGui::Button(axisLabels[index], ImVec2(26.0f, 26.0f)))
            {
                values[index] = resetValue;
                changed = true;
            }

            ImGui::SameLine(0.0f, 4.0f);
            if (ImGui::DragFloat("##Value", &values[index], speed, 0.0f, 0.0f, format))
                changed = true;

            ImGui::PopItemWidth();
        }

        ImGui::Columns(1);
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
            Utils::EndPersistentSection();
    }

    SectionContainer::SectionContainer(const char* id)
        : idScope_(id)
        , background_(ImGuiCol_ChildBg, SectionBackground)
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
