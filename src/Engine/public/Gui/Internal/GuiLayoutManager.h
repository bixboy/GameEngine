#pragma once
#include <array>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Gui/GuiDocking.h"
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
        std::size_t operator()(EditorLayoutType type) const noexcept
        {
            return static_cast<std::size_t>(type);
        }
    };

    /**
     * @brief Manage multiple independent ImGui dock layouts and panel visibility.
     */
    class GuiLayoutManager
    {
    public:
        GuiLayoutManager(GuiSystem& guiSystem, GuiManager& guiManager);

        void Render();
        void Switch(EditorLayoutType newLayout);
        void SaveCurrentLayout();
        void LoadLayout(EditorLayoutType layout);
        void SaveAllLayoutsToDisk();

        enum class LayoutRegistrationMode
        {
            RegisterOnly,
            LoadIfUninitialized,
            ForceLoad
        };

        void RegisterPanels(EditorLayoutType layout, std::span<GuiPanel*> panels, LayoutRegistrationMode mode = LayoutRegistrationMode::RegisterOnly);
        void DetachPanels(std::span<GuiPanel*> panels);
        void ResetLayout(EditorLayoutType layout);
        void SetPanelsForLayout(EditorLayoutType layout, std::span<GuiPanel*> panels);
        void AddPanel(EditorLayoutType layout, GuiPanel& panel);
        void RemovePanel(GuiPanel& panel);

        [[nodiscard]] EditorLayoutType GetCurrentLayout() const noexcept { return currentLayout_; }

    private:
        void ProcessPendingSwitch_();
        void EnsureDockspaceForCurrentLayout_();
        void ApplyPanelVisibility_();
        void RemovePanelFromLayout_(GuiPanel& panel, EditorLayoutType layout);
        void LoadPersistedLayouts_();
        void PersistLayoutsToDisk_();
        
        [[nodiscard]] std::filesystem::path ResolveLayoutStoragePath_() const;
        
        static void TrimTrailingCarriageReturn_(std::string& value);
        static std::string LayoutTypeToString(EditorLayoutType type);
        static std::optional<EditorLayoutType> LayoutTypeFromString(const std::string& value);

        struct StoredLayout
        {
            std::string serialized;
            std::array<ImGuiID, static_cast<std::size_t>(DockSpaceRegion::Count)> dockRegionIds{};
        };

        GuiSystem* guiSystem_{nullptr};
        GuiManager* guiManager_{nullptr};

        EditorLayoutType currentLayout_{EditorLayoutType::Scene};
        std::optional<EditorLayoutType> pendingLayout_{};
        std::unordered_map<EditorLayoutType, StoredLayout, EditorLayoutTypeHash> layoutData_{};
        std::unordered_map<EditorLayoutType, std::string, EditorLayoutTypeHash> dockspaceNames_{};
        std::unordered_map<EditorLayoutType, std::vector<GuiPanel*>, EditorLayoutTypeHash> layoutPanels_{};
        std::unordered_map<GuiPanel*, EditorLayoutType> panelLayoutLookup_{};
        std::unordered_set<EditorLayoutType, EditorLayoutTypeHash> initializedLayouts_{};
        std::unordered_set<EditorLayoutType, EditorLayoutTypeHash> pendingLayoutInitialization_{};

        bool dockspaceDirty_{true};
        bool switchRequested_{false};
        std::filesystem::path layoutStorageFile_{};
    };
}
