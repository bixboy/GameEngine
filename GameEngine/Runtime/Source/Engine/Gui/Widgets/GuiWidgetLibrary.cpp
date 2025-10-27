#include "Engine/Gui/Widgets/GuiWidgetLibrary.h"

#include "Engine/Gui/Utils/GuiHelpers.h"

#include <algorithm>

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

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(kHeaderSpacing, kHeaderSpacing));
        ImGui::TextUnformatted(options.title.c_str());

        if (!options.subtitle.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", options.subtitle.c_str());
            ImGui::PopStyleColor();
        }

        if (options.showSeparator)
            ImGui::Separator();

        ImGui::PopStyleVar();
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
            Gui::Utils::DrawHelpMarker(metric.hint.c_str());
        }
    }

    void DrawMetricsTable(const std::vector<MetricDisplay>& metrics, float columnWidth)
    {
        if (metrics.empty())
            return;

        if (!ImGui::BeginTable("MetricsTable", 2, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg))
            return;

        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

        for (const auto& metric : metrics)
            DrawMetricRow(metric);

        ImGui::EndTable();
    }

    PanelSection::PanelSection(const char* label, bool defaultOpen, ImGuiTreeNodeFlags flags)
    {
        ImGuiTreeNodeFlags localFlags = flags;
        if (defaultOpen)
            localFlags |= ImGuiTreeNodeFlags_DefaultOpen;
        else
            localFlags &= ~ImGuiTreeNodeFlags_DefaultOpen;

        open_ = ImGui::CollapsingHeader(label, localFlags);
    }

    PanelSection::~PanelSection()
    {
        if (open_)
            ImGui::Spacing();
    }

    PanelToolbar::PanelToolbar()
    {
        ImGui::PushID(this);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));
    }

    PanelToolbar::~PanelToolbar()
    {
        if (!committed_)
            Commit();

        ImGui::PopStyleVar();
        ImGui::PopID();
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

        bool drewAny = false;
        bool firstInRow = true;

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

            if (!drewAny)
            {
                drewAny = true;
                firstInRow = true;
            }

            if (!firstInRow)
                ImGui::SameLine();

            draw();
            firstInRow = false;
        }

        if (drewAny)
        {
            ImGui::NewLine();
            ImGui::Separator();
        }
    }

    namespace Builder
    {
        Section::Section(const char* label, bool defaultOpen, ImGuiTreeNodeFlags flags)
            : section_(label, defaultOpen, flags)
        {
        }

        Section::~Section() = default;
    }
}

