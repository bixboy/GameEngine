#include "Gui/Internal/WorkspaceRegistry.h"

#include "Gui/Core/GuiManager.h"

namespace BixEngine::Gui
{
    void WorkspaceRegistry::RegisterWorkspace(Workspace workspace)
    {
        if (workspace.name.empty())
            return;

        workspaces_[workspace.name] = std::move(workspace);
    }

    bool WorkspaceRegistry::ActivateWorkspace(const String& name, GuiManager& manager)
    {
        if (name.empty())
            return false;

        auto it = workspaces_.find(name);
        if (it == workspaces_.end())
            return false;

        if (activeWorkspace_ == name)
        {
            if (it->second.onActivate)
                it->second.onActivate(manager);
            
            return true;
        }

        if (!activeWorkspace_.empty())
        {
            if (auto previous = workspaces_.find(activeWorkspace_); previous != workspaces_.end())
            {
                if (previous->second.onDeactivate)
                    previous->second.onDeactivate(manager);
            }
        }

        activeWorkspace_ = name;

        if (it->second.layout.has_value() && layoutManager_)
            layoutManager_->Switch(*it->second.layout);

        if (it->second.onActivate)
            it->second.onActivate(manager);

        if (!it->second.homePanel.empty())
            manager.GetHistory().SetHome(it->second.homePanel);

        return true;
    }

    const WorkspaceRegistry::Workspace* WorkspaceRegistry::GetActiveWorkspace() const noexcept
    {
        if (activeWorkspace_.empty())
            return nullptr;

        if (auto it = workspaces_.find(activeWorkspace_); it != workspaces_.end())
            return &it->second;

        return nullptr;
    }

    const WorkspaceRegistry::Workspace* WorkspaceRegistry::FindWorkspace(const String& name) const noexcept
    {
        if (auto it = workspaces_.find(name); it != workspaces_.end())
            return &it->second;

        return nullptr;
    }
}
