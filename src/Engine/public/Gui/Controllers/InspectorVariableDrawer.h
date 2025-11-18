#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "Gui/Controllers/BaseAssetEditorController.h"
#include "Gui/Utils/GuiHelpers.h"
#include "imgui.h"

namespace BixEngine::Gui::Inspector
{
    inline void DrawExposedVariablesSection(BaseAssetEditorController::SharedState& state,
                                            std::string_view sectionLabel,
                                            const char* tableId,
                                            const char* emptyMessage = "No exposed variables.")
    {
        auto matchesType = [](std::string_view typeName, std::string_view expectedSuffix)
        {
            if (typeName.size() < expectedSuffix.size())
                return false;

            const std::size_t offset = typeName.size() - expectedSuffix.size();
            if (typeName.compare(offset, expectedSuffix.size(), expectedSuffix) != 0)
                return false;

            if (offset == 0)
                return true;

            const char preceding = typeName[offset - 1];
            return std::isalnum(static_cast<unsigned char>(preceding)) == 0 && preceding != '_';
        };

        auto trimBraces = [](std::string_view input)
        {
            const auto first = input.find_first_not_of(" \t{}\n\r");
            const auto last = input.find_last_not_of(" \t{}\n\r");

            if (first == std::string_view::npos)
                return std::string{};

            return std::string{input.substr(first, last - first + 1)};
        };

        auto extractNumbers = [](std::string_view input)
        {
            std::vector<float> numbers;
            std::string current;
            current.reserve(input.size());

            auto pushCurrent = [&]()
            {
                if (current.empty())
                    return;

                try
                {
                    numbers.push_back(std::stof(current));
                }
                catch (...)
                {
                }

                current.clear();
            };

            for (char ch : input)
            {
                if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '+' || ch == '-' || ch == '.' || ch == 'e' || ch ==
                    'E')
                {
                    current.push_back(ch);
                }
                else
                {
                    pushCurrent();
                }
            }

            pushCurrent();
            return numbers;
        };

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
                const std::string typeName = variable.type.IsEmpty() ? std::string{} : variable.type.Std();
                const std::string rawValue = variable.value.IsEmpty() ? std::string{} : variable.value.Std();
                const std::string cleanedValue = trimBraces(rawValue);
                const auto numbers = extractNumbers(cleanedValue);

                const auto boolValue = [&]()
                {
                    const std::string lower = cleanedValue.empty() ? std::string{} : cleanedValue;
                    std::string lowered = lower;
                    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch)
                    {
                        return static_cast<char>(std::tolower(ch));
                    });

                    if (lowered == "true" || lowered == "1")
                        return true;

                    if (lowered == "false" || lowered == "0")
                        return false;

                    return !numbers.empty() && numbers.front() != 0.0f;
                }();

                const auto drawBool = [&]()
                {
                    bool value = boolValue;
                    if (ImGui::Checkbox("##ExposedValue", &value))
                        variable.value = value ? "true" : "false";
                };

                const auto drawInt = [&]()
                {
                    int value = numbers.empty() ? 0 : static_cast<int>(numbers.front());
                    if (ImGui::InputInt("##ExposedValue", &value))
                        variable.value = std::to_string(value);
                };

                const auto drawFloat = [&]()
                {
                    float value = numbers.empty() ? 0.0f : numbers.front();
                    if (ImGui::InputFloat("##ExposedValue", &value, 0.0f, 0.0f, "%.3f"))
                    {
                        char formatted[64]{};
                        std::snprintf(formatted, sizeof(formatted), "%.3ff", value);
                        variable.value = formatted;
                    }
                };

                const auto drawDouble = [&]()
                {
                    double value = numbers.empty() ? 0.0 : static_cast<double>(numbers.front());
                    if (ImGui::InputDouble("##ExposedValue", &value, 0.0, 0.0, "%.3f"))
                    {
                        char formatted[64]{};
                        std::snprintf(formatted, sizeof(formatted), "%.3f", value);
                        variable.value = formatted;
                    }
                };

                const auto drawVector = [&](int dimension)
                {
                    float values[3]{};
                    for (int i = 0; i < dimension && i < static_cast<int>(numbers.size()); ++i)
                        values[i] = numbers[i];

                    const bool edited = dimension == 2
                        ? ImGui::InputFloat2("##ExposedValue", values, "%.3f")
                        : ImGui::InputFloat3("##ExposedValue", values, "%.3f");

                    if (edited)
                    {
                        char formatted[128]{};
                        if (dimension == 2)
                            std::snprintf(formatted, sizeof(formatted), "{%.3ff, %.3ff}", values[0], values[1]);
                        else
                            std::snprintf(formatted, sizeof(formatted), "{%.3ff, %.3ff, %.3ff}", values[0], values[1], values[2]);

                        variable.value = formatted;
                    }
                };

                bool handled = false;
                if (matchesType(typeName, "bool"))
                {
                    drawBool();
                    handled = true;
                }
                else if (matchesType(typeName, "int") || matchesType(typeName, "int32_t") || matchesType(typeName,
                         "std::int32_t"))
                {
                    drawInt();
                    handled = true;
                }
                else if (matchesType(typeName, "float"))
                {
                    drawFloat();
                    handled = true;
                }
                else if (matchesType(typeName, "double"))
                {
                    drawDouble();
                    handled = true;
                }
                else if (matchesType(typeName, "Math::Vector2") || matchesType(typeName, "Vector2"))
                {
                    drawVector(2);
                    handled = true;
                }
                else if (matchesType(typeName, "Math::Vector3") || matchesType(typeName, "Vector3"))
                {
                    drawVector(3);
                    handled = true;
                }

                if (!handled)
                {
                    std::array<char, 256> buffer{};
                    const std::string_view currentValue = variable.value.View();
                    const std::size_t copyLength = std::min(buffer.size() - 1, currentValue.size());
                    std::memcpy(buffer.data(), currentValue.data(), copyLength);
                    buffer[copyLength] = '\0';

                    if (ImGui::InputText("##ExposedValue", buffer.data(), buffer.size()))
                        variable.value = buffer.data();
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }
}
