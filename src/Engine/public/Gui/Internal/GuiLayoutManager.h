#pragma once
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "imgui.h"
#include "Containers/String.h"
#include "Gui/Core/GuiData.h"

namespace BixEngine::Gui
{
    class GuiManager;
    class GuiPanel;
    class GuiSystem;

    using LayoutID = String;

    namespace DefaultLayouts
    {
        static const LayoutID Scene = "Scene";
        static const LayoutID ActorEditor = "ActorEditor";
    }

    class GuiLayoutManager
    {
    public:
        GuiLayoutManager(GuiSystem& guiSystem, GuiManager& guiManager);
        ~GuiLayoutManager() = default;

        void Update();
        void Render();

        // --- Layout Management ---
        void CreateLayout(const LayoutID& layoutName);
        void Switch(const LayoutID& newLayout);
        
        [[nodiscard]] LayoutID GetCurrentLayout() const noexcept { return currentLayout_; }
        [[nodiscard]] bool HasLayout(const LayoutID& layout) const;

        // --- Persistence ---
        void SaveCurrentLayout();
        void SaveAllLayoutsToDisk();

        enum class LayoutRegistrationMode
        {
            RegisterOnly,
            LoadIfUninitialized,
            ForceLoad
        };

        // --- Panel Management ---
        void RegisterPanels(const LayoutID& layout, std::span<GuiPanel*> panels, LayoutRegistrationMode mode = LayoutRegistrationMode::RegisterOnly);
        void SetPanelsForLayout(const LayoutID& layout, std::span<GuiPanel*> panels);
        
        void AddPanel(const LayoutID& layout, GuiPanel& panel);
        void RemovePanel(GuiPanel& panel);
        void DetachPanels(std::span<GuiPanel*> panels);

        void ResetLayout(const LayoutID& layout);
        void LoadLayout(const LayoutID& layout);

        [[nodiscard]] bool IsPanelVisibleInCurrentLayout(GuiPanel* panel) const;

        void SetMenuPanelFilter(std::function<bool(GuiPanel*)> filter);

    private:
        void ProcessPendingSwitch_();
        void RemovePanelFromLayout_(GuiPanel& panel, const LayoutID& layout);
        void EnsureDockSpaceForCurrentLayout_();
        void ApplyPanelVisibility_();
        void LoadPersistedLayouts_();
        void PersistLayoutsToDisk_();

        struct StoredLayout
        {
            std::string serialized;
            std::array<ImGuiID, static_cast<size_t>(DockSpaceRegion::Count)> dockRegionIds{};
        };

        GuiSystem* guiSystem_{nullptr};
        GuiManager* guiManager_{nullptr};

        LayoutID currentLayout_{DefaultLayouts::Scene};
        std::optional<LayoutID> pendingLayout_;
        
        std::function<bool(GuiPanel*)> customMenuFilter_;

        bool switchRequested_{false};
        bool dockspaceDirty_{true};

        std::filesystem::path layoutStorageFile_;
        
        // Data Structures using String (LayoutID) as key
        std::unordered_map<LayoutID, std::vector<GuiPanel*>> layoutPanels_;
        std::unordered_map<GuiPanel*, LayoutID> panelLayoutLookup_;
        std::unordered_map<LayoutID, StoredLayout> layoutData_;
        
        std::unordered_set<LayoutID> initializedLayouts_;
        std::unordered_set<LayoutID> pendingInitialization_;
    };
}
