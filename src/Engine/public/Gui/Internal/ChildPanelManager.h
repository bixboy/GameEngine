#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include "Containers/String.h"


namespace BixEngine::Gui
{
    class GuiPanel;
    class GuiPanelWindow;

    class ChildPanelManager
    {
    public:

        struct ChildPanelLink
        {
            String panelName;
            bool closeWithParent;
        };

        using ChildList = std::vector<ChildPanelLink>;

        void RegisterChild(GuiPanelWindow& parent, String panelName, bool closeWithParent);

        void UnregisterChildByName(const String& panelName);

        void RemoveChildren(GuiPanelWindow& parent, const std::function<void(const String&)>& onClose);

        [[nodiscard]] const ChildList* GetChildren(const GuiPanelWindow& parent) const noexcept;
    
        void Clear() noexcept;

    private:
        using MapType = std::unordered_map<GuiPanelWindow*, ChildList>;

        MapType children_{};
    };
}
