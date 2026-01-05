#include "imgui.h"


namespace BixEngine::Gui::Utils
{
    void DrawPropertyLabel(const char* label, float columnWidth) noexcept
    {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label ? label : "");
        ImGui::NextColumn();
    }

    void DrawHelpTooltip(const char* text) noexcept
    {
        if (!text || text[0] == '\0')
            return;

        ImGui::TextDisabled("%s", "?");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            ImGui::SetTooltip("%s", text);
    }

    ImVec2 ComputeBadgeSize(const char* text, const ImVec2& padding) noexcept
    {
        if (!text || text[0] == '\0')
            return {0.0f, 0.0f};

        const ImVec2 textSize = ImGui::CalcTextSize(text);
        return {textSize.x + padding.x * 2.0f, textSize.y + padding.y * 2.0f};
    }
}
