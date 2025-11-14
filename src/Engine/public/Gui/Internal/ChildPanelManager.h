#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "Containers/String.h"

namespace BixEngine::Gui
{
    class GuiPanelController;

    struct ChildPanelLink
    {
        String panelName;
        bool closeWithParent{true};
    };

    /**
     * @brief Gestion centralisée des panneaux enfants liés à un contrôleur parent.
     */
    class ChildPanelManager
    {
    public:
        void RegisterChild(GuiPanelController& parent, String panelName, bool closeWithParent);
        void UnregisterChildByName(const String& panelName);
        void RemoveChildren(GuiPanelController& parent, const std::function<void(const String&)>& onClose);
        [[nodiscard]] const std::vector<ChildPanelLink>* GetChildren(const GuiPanelController& parent) const noexcept;
        void Clear() noexcept;

    private:
        std::unordered_map<GuiPanelController*, std::vector<ChildPanelLink>> children_{};
    };
}
