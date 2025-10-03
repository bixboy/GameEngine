#pragma once

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

namespace Engine::Gui
{
    struct ActionTooltipAction
    {
        std::string Label{};
        std::function<void()> Callback{};

        ActionTooltipAction() = default;
        ActionTooltipAction(std::string label, std::function<void()> callback)
            : Label(std::move(label)), Callback(std::move(callback))
        {
        }
    };

    void ShowActionTooltip(std::string_view header, std::initializer_list<ActionTooltipAction> actions);
    void ShowActionTooltip(std::string_view header, const ActionTooltipAction* actions, std::size_t count);
}
