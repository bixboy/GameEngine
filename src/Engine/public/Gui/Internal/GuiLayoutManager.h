#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Gui/GuiCommon.h"
#include "imgui.h"


namespace BixEngine::Gui
{
    class GuiManager;
    class GuiPanel;
    class GuiSystem;

    enum class EditorLayoutType
    {
        Scene,
        ActorEditor
    };

    struct EditorLayoutTypeHash
    {
        size_t operator()(EditorLayoutType type) const noexcept
        {
            return static_cast<size_t>(type);
        }
    };

    /**
     * @brief Gère indépendamment plusieurs ImGui dock layouts et panel.
     */
    class GuiLayoutManager
    {
    public:
        GuiLayoutManager(GuiSystem& guiSystem, GuiManager& guiManager);

        void Render();

        void Switch(EditorLayoutType newLayout);
        void SaveCurrentLayout();
        void SaveAllLayoutsToDisk();

        enum class LayoutRegistrationMode
        {
            RegisterOnly,
            LoadIfUninitialized,
            ForceLoad
        };

        void RegisterPanels(EditorLayoutType layout, std::span<GuiPanel*> panels, LayoutRegistrationMode mode = LayoutRegistrationMode::RegisterOnly);

        void ResetLayout(EditorLayoutType layout);
        void LoadLayout(EditorLayoutType layout);

        void SetPanelsForLayout(EditorLayoutType layout, std::span<GuiPanel*> panels);
        void AddPanel(EditorLayoutType layout, GuiPanel& panel);
        void RemovePanel(GuiPanel& panel);
        void DetachPanels(std::span<GuiPanel*> panels);

        [[nodiscard]] EditorLayoutType GetCurrentLayout() const noexcept { return currentLayout_; }

    private:
        
        // ---- Internal lifecycle ----
        void ProcessPendingSwitch_();
        void EnsureDockspaceForCurrentLayout_();
        void ApplyPanelVisibility_();

        // ---- Panel logic ----
        void RemovePanelFromLayout_(GuiPanel& panel, EditorLayoutType layout);

        // ---- Persistence ----
        void LoadPersistedLayouts_();
        void PersistLayoutsToDisk_();

        // ---- Utility ----
        static std::string LayoutTypeToString(EditorLayoutType type);
        static std::optional<EditorLayoutType> LayoutTypeFromString(const std::string& value);

        struct StoredLayout
        {
            std::string serialized;
            std::array<ImGuiID, static_cast<size_t>(DockSpaceRegion::Count)> dockRegionIds{};
        };

        GuiSystem* guiSystem_ = nullptr;
        GuiManager* guiManager_ = nullptr;

        EditorLayoutType currentLayout_ = EditorLayoutType::Scene;
        std::optional<EditorLayoutType> pendingLayout_;

        std::unordered_map<EditorLayoutType, StoredLayout, EditorLayoutTypeHash> layoutData_;
        std::unordered_map<EditorLayoutType, std::string, EditorLayoutTypeHash> dockspaceNames_;
        std::unordered_map<EditorLayoutType, std::vector<GuiPanel*>, EditorLayoutTypeHash> layoutPanels_;
        std::unordered_map<GuiPanel*, EditorLayoutType> panelLayoutLookup_;

        std::unordered_set<EditorLayoutType, EditorLayoutTypeHash> initializedLayouts_;
        std::unordered_set<EditorLayoutType, EditorLayoutTypeHash> pendingInitialization_;

        bool dockspaceDirty_ = true;
        bool switchRequested_ = false;

        std::filesystem::path layoutStorageFile_;
    };
}
