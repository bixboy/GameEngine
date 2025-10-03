#include "Gui/Widgets/ActionTooltip.h"

#include "imgui.h"

namespace Engine::Gui
{
    namespace
    {
        void DrawActionButtons(const ActionTooltipAction* actions, std::size_t count)
        {
            ImGui::Indent(8.0f);
            for (std::size_t index = 0; index < count; ++index)
            {
                const ActionTooltipAction& action = actions[index];
                if (action.Label.IsEmpty())
                    continue;

                ImGui::PushID(static_cast<int>(index));
                ImGui::TextDisabled("%s", action.Label.c_str());

                if (ImGui::IsItemHovered())
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

                if (action.Callback && ImGui::IsItemClicked())
                    action.Callback();

                ImGui::PopID();
            }
            ImGui::Unindent(8.0f);
        }
    }

    void ShowActionTooltip(const String& header, std::initializer_list<ActionTooltipAction> actions)
    {
        ShowActionTooltip(header, actions.begin(), actions.size());
    }

    void ShowActionTooltip(const String& header, const ActionTooltipAction* actions, std::size_t count)
    {
        if (!ImGui::BeginTooltip())
            return;

        const bool hasHeader = !header.IsEmpty();
        if (hasHeader)
        {
            const auto headerView = header.View();
            ImGui::TextUnformatted(headerView.data(), headerView.data() + headerView.size());
        }

        if (count > 0)
        {
            if (hasHeader)
                ImGui::Separator();

            DrawActionButtons(actions, count);
        }
        else if (hasHeader)
        {
            ImGui::TextDisabled("Aucune action disponible.");
        }

        ImGui::EndTooltip();
    }
}
