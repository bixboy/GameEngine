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
        bool closeWithParent = true;
    };

    /**
     * @brief Gestion centralisée des panels enfants appartenant à un contrôleur parent.
     */
    class ChildPanelManager
    {
    public:
        void RegisterChild(GuiPanelController& parent, String panelName, bool closeWithParent);

        void UnregisterChildByName(const String& panelName);

        void RemoveChildren(GuiPanelController& parent, const std::function<void(const String&)>& onClose);

        [[nodiscard]] const std::vector<ChildPanelLink>*
        GetChildren(const GuiPanelController& parent) const noexcept;

        void Clear() noexcept;

    private:
        using ChildList = std::vector<ChildPanelLink>;
        using MapType = std::unordered_map<GuiPanelController*, ChildList>;

        MapType children_{};
    };
}
