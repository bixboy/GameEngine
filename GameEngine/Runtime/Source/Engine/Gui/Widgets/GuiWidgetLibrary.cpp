#include "Engine/Gui/Widgets/GuiWidgetLibrary.h"
#include "Engine/Gui/Utils/GuiHelpers.h"


namespace BixEngine::Gui::Widgets
{
    namespace
    {
        constexpr float kHeaderSpacing = 4.0f;
    }

    // ─────────────────────────────────────────────
    // 🎨  En-tête de panneau (titre + sous-titre)
    // ─────────────────────────────────────────────
    
    void DrawPanelHeader(const PanelHeaderOptions& options)
    {
        if (options.title.empty())
            return;

        ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(kHeaderSpacing, kHeaderSpacing));
        ImGui::TextUnformatted(options.title.c_str());

        if (!options.subtitle.empty())
        {
            ScopedColor disabled(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", options.subtitle.c_str());
        }

        if (options.showSeparator)
            ImGui::Separator();
    }

    // ─────────────────────────────────────────────
    // 📊  Ligne d'une métrique dans le tableau
    // ─────────────────────────────────────────────
    
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
            Utils::DrawHelpMarker(metric.hint.c_str());
        }
    }

    // ─────────────────────────────────────────────
    // 📋  Tableau complet de métriques
    // ─────────────────────────────────────────────
    
    void DrawMetricsTable(const std::vector<MetricDisplay>& metrics, float columnWidth, const char* id)
    {
        if (metrics.empty())
            return;

        ImGui::PushID(id);

        if (ImGui::BeginTable("MetricsTable", 2,
            ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            for (const auto& metric : metrics)
                DrawMetricRow(metric);

            ImGui::EndTable();
        }

        ImGui::PopID();
    }

    // ─────────────────────────────────────────────
    // 📦  Section repliable (RAII)
    // ─────────────────────────────────────────────
    
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

    // ─────────────────────────────────────────────
    // 🧰  Barre d’outils horizontale (RAII)
    // ─────────────────────────────────────────────
    
    PanelToolbar::PanelToolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));
    }

    PanelToolbar::~PanelToolbar()
    {
        ImGui::PopStyleVar();
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

        bool firstInRow = true;
        bool drewAny = false;

        // 🔹 Côté gauche
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

        // 🔹 Côté droit
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

    // ─────────────────────────────────────────────
    // 🧱  Builder::Section (inline namespace)
    // ─────────────────────────────────────────────
    
    Section::Section(const char* label, bool defaultOpen, ImGuiTreeNodeFlags flags): section_(label, defaultOpen, flags)
    {
    }

    Section::~Section() = default;
}
