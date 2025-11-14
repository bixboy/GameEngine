#pragma once

#include <functional>
#include <optional>
#include <unordered_map>

#include "Containers/String.h"
#include "Gui/Internal/GuiLayoutManager.h"

namespace BixEngine::Gui
{
    class GuiManager;

    class WorkspaceRegistry
    {
    public:
        struct Workspace
        {
            String name{};
            std::function<void(GuiManager&)> onActivate{};
            std::function<void(GuiManager&)> onDeactivate{};
            std::optional<EditorLayoutType> layout{};
            String homePanel{};
        };

        void SetLayoutManager(GuiLayoutManager* layoutManager) noexcept { layoutManager_ = layoutManager; }

        void RegisterWorkspace(Workspace workspace);
        bool ActivateWorkspace(const String& name, GuiManager& manager);

        [[nodiscard]] const Workspace* GetActiveWorkspace() const noexcept;
        [[nodiscard]] const Workspace* FindWorkspace(const String& name) const noexcept;

    private:
        std::unordered_map<String, Workspace> workspaces_{};
        GuiLayoutManager* layoutManager_{nullptr};
        String activeWorkspace_{};
    };
}
