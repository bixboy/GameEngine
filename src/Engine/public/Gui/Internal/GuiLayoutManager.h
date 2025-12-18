#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

#include "Gui/Core/GuiCommon.h"
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

        void Update();
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

        void SetMenuPanelFilter(std::function<bool(GuiPanel*)> filter) { menuPanelFilter_ = std::move(filter); }

        [[nodiscard]] EditorLayoutType GetCurrentLayout() const noexcept { return currentLayout_; }

    private:
        
        // ---- Internal lifecycle ----
        void ProcessPendingSwitch_();
        void EnsureDockspaceForCurrentLayout_();
        void ApplyPanelVisibility_();
        void DrawMainMenuBar_();
        void DrawSaveAsDialog_();
        void DrawOpenSceneDialog_();
        void DrawDeleteSceneDialog_();
        void DrawRenameSceneDialog_();
        void DrawCloseSceneConfirmation_();
        
        void AddToRecentScenes_(const std::filesystem::path& path);
        void LoadRecentScenes_();
        void SaveRecentScenes_();

        void RemovePanelFromLayout_(GuiPanel& panel, EditorLayoutType layout);



        void LoadPersistedLayouts_();
        void PersistLayoutsToDisk_();

        static std::string LayoutTypeToString(EditorLayoutType type);
        static std::optional<EditorLayoutType> LayoutTypeFromString(const std::string& v);

        struct StoredLayout
        {
            std::string serialized;
            std::array<ImGuiID, static_cast<size_t>(DockSpaceRegion::Count)> dockRegionIds{};
        };

        GuiSystem* guiSystem_{nullptr};
        GuiManager* guiManager_{nullptr};
        std::function<bool(GuiPanel*)> menuPanelFilter_;

        EditorLayoutType currentLayout_{EditorLayoutType::Scene};
        std::optional<EditorLayoutType> pendingLayout_;
        bool switchRequested_{false};
        bool dockspaceDirty_{true};

        std::filesystem::path layoutStorageFile_;
        std::unordered_map<EditorLayoutType, std::vector<GuiPanel*>> layoutPanels_;
        std::unordered_map<GuiPanel*, EditorLayoutType> panelLayoutLookup_;
        std::unordered_map<EditorLayoutType, std::string> dockspaceNames_;
        std::unordered_map<EditorLayoutType, StoredLayout> layoutData_;
        std::unordered_set<EditorLayoutType> initializedLayouts_;
        std::unordered_set<EditorLayoutType> pendingInitialization_;

        // Scene Management State
        std::filesystem::path currentScenePath_;
        bool showSaveAsDialog_{false};
        bool showOpenSceneDialog_{false};
        bool showDeleteSceneDialog_{false};
        bool showRenameSceneDialog_{false};
        bool showCloseSceneConfirmation_{false};
        char saveAsFilenameBuffer_[256]{};
        char renameFilenameBuffer_[256]{};
        
        // Recent Scenes
        std::vector<std::filesystem::path> recentScenes_;
        std::filesystem::path recentScenesFile_;
        std::filesystem::path sceneToDelete_;
        
        // Scene State Tracking
        bool isSceneDirty_{false};
        
        bool showEditorPreferences_{false};
    };
}
