#pragma once

#include <array>
#include <cstring>
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

            for (std::size_t index = 0; index < state.exposedVariables.size(); ++index)
            {
                auto& variable = state.exposedVariables[index];

                ImGui::TableNextRow();

                ImGui::PushID(static_cast<int>(index));

                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(variable.name.IsEmpty() ? "<Unnamed>" : variable.name.View().data());

                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(variable.type.IsEmpty() ? "Unknown" : variable.type.View().data());

                ImGui::TableSetColumnIndex(2);
                std::array<char, 256> buffer{};
                const std::string_view currentValue = variable.value.View();
                const std::size_t copyLength = std::min(buffer.size() - 1, currentValue.size());
                std::memcpy(buffer.data(), currentValue.data(), copyLength);
                buffer[copyLength] = '\0';

                if (ImGui::InputText("##ExposedValue", buffer.data(), buffer.size()))
                    variable.value = buffer.data();

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }
}
