#include "Gui/Widgets/ActionTooltip.h"

#include "imgui.h"

#include <cfloat>

namespace Engine::Gui
{
    namespace
    {
        void DrawActionButtons(const ActionTooltipAction* actions, std::size_t count)
        {
            for (std::size_t index = 0; index < count; ++index)
            {
                const ActionTooltipAction& action = actions[index];
                if (action.Label.empty())
                    continue;

                ImGui::PushID(static_cast<int>(index));
                const bool clicked = ImGui::Button(action.Label.c_str(), ImVec2(-FLT_MIN, 0.0f));
                if (clicked && action.Callback)
                {
                    action.Callback();
                }
                ImGui::PopID();
            }
        }
    }

    void ShowActionTooltip(std::string_view header, std::initializer_list<ActionTooltipAction> actions)
    {
        ShowActionTooltip(header, actions.begin(), actions.size());
    }

    void ShowActionTooltip(std::string_view header, const ActionTooltipAction* actions, std::size_t count)
    {
        if (!ImGui::BeginTooltip())
            return;

        const bool hasHeader = !header.empty();
        if (hasHeader)
        {
            ImGui::TextUnformatted(header.data(), header.data() + header.size());
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
