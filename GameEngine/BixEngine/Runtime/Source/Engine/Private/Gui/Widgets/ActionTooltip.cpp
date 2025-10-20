#include "Gui/Widgets/ActionTooltip.h"

#include "imgui.h"

namespace BixEngine::Gui
{
    void ShowActionTooltip(const String& header, std::initializer_list<ActionTooltipAction> actions)
    {
        ShowActionTooltip(header, actions.begin(), actions.size());
    }

    void ShowActionTooltip(const String& header, const ActionTooltipAction* actions, std::size_t count)
    {
        if (!ImGui::BeginTooltip())
            return;

        (void)actions;
        (void)count;

        const auto headerView = header.View();
        ImGui::TextUnformatted(headerView.data(), headerView.data() + headerView.size());

        ImGui::EndTooltip();
    }
}
