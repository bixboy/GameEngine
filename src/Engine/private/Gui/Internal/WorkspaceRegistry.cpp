#include "Gui/Internal/WorkspaceRegistry.h"

#include "Gui/GuiManager.h"

namespace BixEngine::Gui
{
    void WorkspaceRegistry::RegisterWorkspace(Workspace workspace)
    {
        if (workspace.name.IsEmpty())
            return;

        workspaces_[workspace.name] = std::move(workspace);
    }

    bool WorkspaceRegistry::ActivateWorkspace(const String& name, GuiManager& manager)
    {
        if (name.IsEmpty())
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

        if (!activeWorkspace_.IsEmpty())
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

        if (!it->second.homePanel.IsEmpty())
            manager.GetHistory().SetHome(it->second.homePanel);

        return true;
    }

    const WorkspaceRegistry::Workspace* WorkspaceRegistry::GetActiveWorkspace() const noexcept
    {
        if (activeWorkspace_.IsEmpty())
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
