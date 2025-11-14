#include "Gui/Internal/ChildPanelManager.h"

#include <algorithm>

#include "Gui/Controllers/GuiPanelController.h"

namespace BixEngine::Gui
{
    void ChildPanelManager::RegisterChild(GuiPanelController& parent, String panelName, bool closeWithParent)
    {
        auto& children = children_[&parent];

        const auto it = std::find_if(children.begin(), children.end(), [&panelName](const ChildPanelLink& link)
                                     { return link.panelName == panelName; });
        if (it != children.end())
        {
            it->closeWithParent = closeWithParent;
            return;
        }

        children.push_back(ChildPanelLink{std::move(panelName), closeWithParent});
    }

    void ChildPanelManager::UnregisterChildByName(const String& panelName)
    {
        for (auto it = children_.begin(); it != children_.end();)
        {
            auto& links = it->second;
            links.erase(std::remove_if(links.begin(), links.end(), [&panelName](const ChildPanelLink& link)
                                       { return link.panelName == panelName; }), links.end());

            if (links.empty())
                it = children_.erase(it);
            else
                ++it;
        }
    }

    void ChildPanelManager::RemoveChildren(GuiPanelController& parent, const std::function<void(const String&)>& onClose)
    {
        auto it = children_.find(&parent);
        if (it == children_.end())
            return;

        if (onClose)
        {
            for (const auto& child : it->second)
            {
                if (child.closeWithParent)
                    onClose(child.panelName);
            }
        }

        children_.erase(it);
    }

    const std::vector<ChildPanelLink>* ChildPanelManager::GetChildren(const GuiPanelController& parent) const noexcept
    {
        if (auto it = children_.find(const_cast<GuiPanelController*>(&parent)); it != children_.end())
            return &it->second;

        return nullptr;
    }

    void ChildPanelManager::Clear() noexcept
    {
        children_.clear();
    }
}
