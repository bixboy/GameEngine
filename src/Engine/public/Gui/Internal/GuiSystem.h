#pragma once
#include <array>
#include <string>
#include <vector>
#include <SDL3/SDL_events.h>
#include "Gui/Core/GuiData.h"
#include "imgui.h"

struct SDL_Window;
struct SDL_Renderer;

namespace BixEngine::Gui
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

        // --- Lifecycle ---
        bool Initialize(SDL_Window* window, SDL_Renderer* renderer);
        void Shutdown() noexcept;

        // --- Rendering Loop ---
        void BeginFrame();
        void EndFrame();
        void Render();

        // --- Docking System ---
        [[nodiscard]] bool IsDockingEnabled() const noexcept { return dockingEnabled_; }
        [[nodiscard]] ImGuiID GetDockspaceId() const noexcept { return dockspaceId_; }

        void SetDockspaceIdentifiers(std::string windowName, std::string dockspaceLabel);
        void RequestDefaultDockLayout();
        
        [[nodiscard]] const std::array<ImGuiID, static_cast<std::size_t>(DockSpaceRegion::Count)>& GetDockRegionIds() const noexcept
        {
            return dockRegionIds_;
        }
        
        void SetDockRegionIds(const std::array<ImGuiID, static_cast<std::size_t>(DockSpaceRegion::Count)>& ids) noexcept;
        
        [[nodiscard]] std::string SaveLayoutToMemory() const;
        void LoadLayoutFromMemory(const std::string& data);

        // --- Panel Management ---
        void RegisterPanel(GuiPanel& panel);
        void UnregisterPanel(GuiPanel& panel);
        void EnqueueDockUpdate(GuiPanel& panel);

        // --- Events & Config ---
        void ProcessEvent(const SDL_Event& event);
        void OnResize(int width, int height);
        [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }

        // --- Debug & Styling ---
        void DumpGuiState() const;
        void ToggleDockDebugOverlay(bool enable) noexcept { bShowDockDebugOverlay_ = enable; }
        
        void SetDockspaceTopPadding(float padding) noexcept { dockspaceTopPadding_ = padding < 0.0f ? 0.0f : padding; }

    private:
        // --- Internal Docking Logic ---
        void BeginDockspaceLayout_();
        
        void BuildDefaultDockLayout_(ImGuiViewport& viewport, ImGuiID dockspaceId, ImGuiDockNodeFlags dockspaceFlags,
            const ImVec2& dockspacePos, const ImVec2& dockspaceSize);
                                     
        void ApplyDockingPreferences_();

        void QueuePanelForDockUpdate_(GuiPanel& panel);
        void RemovePanelFromDockQueue_(GuiPanel& panel);
        void QueueAllPanelsForDockUpdate_();

        [[nodiscard]] bool HasSavedDockLayout_() const;

        // --- Data ---
        SDL_Window* window_{nullptr};
        SDL_Renderer* renderer_{nullptr};

        bool initialized_{false};
        bool frameBegun_{false};
        bool dockingEnabled_{false};
        
        // Docking State
        bool dockLayoutBuilt_{false};
        bool rebuildDockLayout_{true};
        bool useSavedDockLayout_{false};
        bool bShowDockDebugOverlay_{false};

        ImGuiID dockspaceId_{0};
        std::string dockspaceWindowName_{"EngineDockSpace"};
        std::string dockspaceLabel_{"EngineDockSpace::DockSpace"};
        
        // IDs des régions (Left, Right, Center...)
        std::array<ImGuiID, static_cast<std::size_t>(DockSpaceRegion::Count)> dockRegionIds_{};

        std::vector<GuiPanel*> panels_{};
        std::vector<GuiPanel*> pendingDockUpdates_{};
        float dockspaceTopPadding_{0.0f};
    };
}