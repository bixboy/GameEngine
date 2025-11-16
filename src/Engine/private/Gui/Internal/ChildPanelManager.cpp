#include "Gui/Internal/ChildPanelManager.h"
#include <algorithm>
#include <utility>
#include "Gui/Controllers/GuiPanelController.h"

namespace BixEngine::Gui
{
    // --------------------------------------------------------------
    // RegisterChild
    // --------------------------------------------------------------
    void ChildPanelManager::RegisterChild(GuiPanelController& parent, String panelName, bool closeWithParent)
    {
        auto& list = children_[&parent];

        if (auto it = std::find_if(list.begin(), list.end(),
            [&](const ChildPanelLink& link)
            {
                return link.panelName == panelName;
            });
            
            it != list.end())
        {
            it->closeWithParent = closeWithParent;
            return;
        }

        list.push_back({std::move(panelName), closeWithParent});
    }

    // --------------------------------------------------------------
    // UnregisterChildByName
    // --------------------------------------------------------------
    void ChildPanelManager::UnregisterChildByName(const String& panelName)
    {
        for (auto it = children_.begin(); it != children_.end();)
        {
            auto& list = it->second;

            std::erase_if(list, [&](const ChildPanelLink& link)
            {
                return link.panelName == panelName;
            });

            if (list.empty())
                it = children_.erase(it);
            else
                ++it;
        }
    }

    // --------------------------------------------------------------
    // RemoveChildren
    // --------------------------------------------------------------
    void ChildPanelManager::RemoveChildren(GuiPanelController& parent, const std::function<void(const String&)>& onClose)
    {
        auto it = children_.find(&parent);
        if (it == children_.end())
            return;

        const auto& list = it->second;

        if (onClose)
        {
            for (const auto& child : list)
            {
                if (child.closeWithParent)
                    onClose(child.panelName);
            }
        }

        children_.erase(it);
    }

    // --------------------------------------------------------------
    // GetChildren
    // --------------------------------------------------------------
    const std::vector<ChildPanelLink>*
    ChildPanelManager::GetChildren(const GuiPanelController& parent) const noexcept
    {
        auto it = children_.find(const_cast<GuiPanelController*>(&parent));
        return it != children_.end() ? &it->second : nullptr;
    }

    // --------------------------------------------------------------
    // Clear
    // --------------------------------------------------------------
    void ChildPanelManager::Clear() noexcept
    {
        children_.clear();
    }
}
