#pragma once

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <utility>

#include "Core/String.h"

namespace Engine::Gui
{
    struct ActionTooltipAction
    {
        String Label{};
        std::function<void()> Callback{};

        ActionTooltipAction() = default;
        ActionTooltipAction(String label, std::function<void()> callback)
            : Label(std::move(label)), Callback(std::move(callback))
        {
        }
    };

    void ShowActionTooltip(const String& header, std::initializer_list<ActionTooltipAction> actions);
    void ShowActionTooltip(const String& header, const ActionTooltipAction* actions, std::size_t count);
}
