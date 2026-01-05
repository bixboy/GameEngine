#include "Gui/Widgets/Metrics/MetricsTable.h"
#include <cstdio>
#include "Gui/Utils/TextHelpers.h"


namespace BixEngine::Gui::Widgets
{
    namespace
    {
        constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV;
    }

    bool MetricsTable::Begin(const char* id, float labelColumnWidth)
    {
        if (ImGui::BeginTable(id, 2, kTableFlags))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, labelColumnWidth);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            
            return true;
        }
        
        return false;
    }

    void MetricsTable::End()
    {
        ImGui::EndTable();
    }

    void MetricsTable::Draw(const char* label, const char* value, const ImVec4& color, const char* tooltip)
    {
        ImGui::TableNextRow();
        
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(label);
        
        if (tooltip)
        {
            ImGui::SameLine();
            Utils::DrawHelpTooltip(tooltip);
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(color, "%s", value);
    }

    void MetricsTable::DrawFloat(const char* label, float value, const char* format, const ImVec4* colorOverride, const char* tooltip)
    {
        char buffer[64];
        (void)snprintf(buffer, sizeof(buffer), format, value);
        
        ImVec4 col = colorOverride ? *colorOverride : ImVec4(1,1,1,1);
        
        Draw(label, buffer, col, tooltip);
    }

    void MetricsTable::DrawInt(const char* label, int value, const ImVec4& color, const char* tooltip)
    {
        char buffer[32];
        (void)snprintf(buffer, sizeof(buffer), "%d", value);
        
        Draw(label, buffer, color, tooltip);
    }

    void MetricsTable::DrawMetricsTable(std::span<const MetricDisplay> metrics, float labelColumnWidth)
    {
        if (Begin("MetricsTable", labelColumnWidth))
        {
            for (const auto& metric : metrics)
            {
                Draw(metric.label.c_str(), metric.value.c_str(), metric.color, metric.tooltip.empty() ? nullptr : metric.tooltip.c_str());
            }
            End();
        }
    }
}
