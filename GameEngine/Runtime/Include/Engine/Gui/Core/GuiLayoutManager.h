#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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

        void SetPanelsForLayout(EditorLayoutType layout, const std::vector<GuiPanel*>& panels);
        void AddPanel(EditorLayoutType layout, GuiPanel& panel);
        void RemovePanel(GuiPanel& panel);

        [[nodiscard]] EditorLayoutType GetCurrentLayout() const noexcept { return currentLayout_; }

    private:
        void ProcessPendingSwitch_();
        void EnsureDockspaceForCurrentLayout_();
        void ApplyPanelVisibility_();
        void RemovePanelFromLayout_(GuiPanel& panel, EditorLayoutType layout);

    private:
        GuiSystem* guiSystem_{nullptr};
        GuiManager* guiManager_{nullptr};

        EditorLayoutType currentLayout_{EditorLayoutType::Scene};
        std::optional<EditorLayoutType> pendingLayout_{};
        std::unordered_map<EditorLayoutType, std::string, EditorLayoutTypeHash> layoutData_{};
        std::unordered_map<EditorLayoutType, std::string, EditorLayoutTypeHash> dockspaceNames_{};
        std::unordered_map<EditorLayoutType, std::vector<GuiPanel*>, EditorLayoutTypeHash> layoutPanels_{};
        std::unordered_map<GuiPanel*, EditorLayoutType> panelLayoutLookup_{};

        bool dockspaceDirty_{true};
        bool switchRequested_{false};
    };
}

