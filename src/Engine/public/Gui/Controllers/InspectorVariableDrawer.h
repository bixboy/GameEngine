#pragma once

#include <string>
#include <string_view>

#include "Gui/Controllers/BaseAssetEditorController.h"
#include "Gui/Utils/GuiHelpers.h"
#include "imgui.h"

namespace BixEngine::Gui::Inspector
{
    inline void DrawExposedVariablesSection(const BaseAssetEditorController::SharedState& state,
                                            std::string_view sectionLabel,
                                            const char* tableId,
                                            const char* emptyMessage = "No exposed variables.")
    {
        std::string headerLabel = sectionLabel.empty() ? std::string{"Variables"} : std::string{sectionLabel};
        Utils::DrawSectionHeader(headerLabel.c_str());

        if (state.exposedVariables.empty())
        {
            Utils::DrawEmptyStateMessage(emptyMessage ? emptyMessage : "No entries to display.");
            return;
        }

        const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp;
        if (ImGui::BeginTable(tableId ? tableId : "ExposedVariablesTable", 3, flags))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.35f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.30f);
            ImGui::TableSetupColumn("Default", ImGuiTableColumnFlags_WidthStretch, 0.35f);
            ImGui::TableHeadersRow();

            for (const auto& variable : state.exposedVariables)
            {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(variable.name.IsEmpty() ? "<Unnamed>" : variable.name.View().data());

                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(variable.type.IsEmpty() ? "Unknown" : variable.type.View().data());

                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(variable.defaultValue.IsEmpty() ? "—" : variable.defaultValue.View().data());
            }

            ImGui::EndTable();
        }
    }
}
