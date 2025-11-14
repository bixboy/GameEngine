#include "Gui/Widgets/Metrics/MetricsTable.h"

#include "Gui/Widgets/Controls/TextHelpers.h"
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    namespace
    {
        constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_SizingStretchSame |
                                                ImGuiTableFlags_NoSavedSettings |
                                                ImGuiTableFlags_RowBg;
    }

    void DrawMetricRow(const MetricDisplay& metric)
    {
        if (metric.label.empty())
            return;

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(metric.label.c_str());

        ImGui::TableSetColumnIndex(1);
        ImGui::TextColored(metric.color, "%s", metric.value.c_str());

        if (!metric.hint.empty())
        {
            ImGui::SameLine();
            Controls::TextHelpers::DrawHelpTooltip(metric.hint.c_str());
        }
    }

    void DrawMetricsTable(const std::vector<MetricDisplay>& metrics, float columnWidth, const char* id)
    {
        if (metrics.empty())
            return;

        ImGui::PushID(id);

        if (ImGui::BeginTable("MetricsTable", 2, kTableFlags))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            for (const auto& metric : metrics)
                DrawMetricRow(metric);

            ImGui::EndTable();
        }

        ImGui::PopID();
    }
}
