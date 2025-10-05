#pragma once

#include <array>
#include <vector>

#include <SDL3/SDL_events.h>

#include "imgui.h"

#include "Gui/GuiDocking.h"

struct SDL_Window;
struct SDL_Renderer;

namespace Engine::Gui
{
    class GuiPanel;

    class GuiSystem
    {
        public:
            GuiSystem() = default;
            ~GuiSystem();

            GuiSystem(const GuiSystem&) = delete;
            GuiSystem& operator=(const GuiSystem&) = delete;
            GuiSystem(GuiSystem&&) noexcept = delete;
            GuiSystem& operator=(GuiSystem&&) noexcept = delete;

            bool Initialize(SDL_Window* window, SDL_Renderer* renderer);
            void Shutdown() noexcept;

            void BeginFrame();
            void EndFrame();
            void Render();

            void ProcessEvent(const SDL_Event& event);

            void RegisterPanel(GuiPanel& panel);
            void UnregisterPanel(GuiPanel& panel);
            void EnqueueDockUpdate(GuiPanel& panel);

            [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }
            [[nodiscard]] bool IsDockingEnabled() const noexcept { return dockingEnabled_; }
            [[nodiscard]] ImGuiID GetDockspaceId() const noexcept { return dockspaceId_; }
            [[nodiscard]] ImGuiID GetRegionDockId(DockSpaceRegion region) const noexcept;
            void RequestDockLayoutRebuild() noexcept;

        private:
            void BeginDockspaceLayout_();
            void BuildDefaultDockLayout_(ImGuiViewport& viewport, ImGuiID dockspaceId, ImGuiDockNodeFlags dockspaceFlags);
            void ApplyDockingPreferences_();
            void QueuePanelForDockUpdate_(GuiPanel& panel);
            void RemovePanelFromDockQueue_(GuiPanel& panel);
            void QueueAllPanelsForDockUpdate_();

            SDL_Window* window_{nullptr};
            SDL_Renderer* renderer_{nullptr};
            bool initialized_{false};
            bool frameBegun_{false};

            bool dockingEnabled_{false};
            bool dockLayoutBuilt_{false};
            bool rebuildDockLayout_{true};
            ImGuiID dockspaceId_{0};
            std::array<ImGuiID, static_cast<std::size_t>(DockSpaceRegion::Count)> dockRegionIds_{};
            std::vector<GuiPanel*> pendingDockUpdates_{};

            std::vector<GuiPanel*> panels_{};
    };
}
