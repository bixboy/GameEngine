#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Core/Containers/String.h"
#include "Engine/Gui/Controllers/BaseAssetEditorController.h"
#include "Engine/Gui/Internal/GuiLayoutManager.h"

namespace BixEngine::Gui
{
    class GuiManager;
    class GuiPanel;
}

namespace BixEngine::Core
{
    class GuiAssetEditorManager
    {
    public:
        using FocusRequestCallback = std::function<void(const std::string&)>;
        using FocusSceneCallback = std::function<void()>;

        GuiAssetEditorManager(Gui::GuiManager& guiManager, Gui::GuiLayoutManager* layoutManager, FocusRequestCallback focusRequestCallback, FocusSceneCallback focusSceneCallback);

        void SetLayoutManager(Gui::GuiLayoutManager* layoutManager) noexcept;
        void SetFocusCallbacks(FocusRequestCallback focusRequestCallback, FocusSceneCallback focusSceneCallback);

        void OpenAssetEditor(const std::filesystem::path& path);
        void CloseAssetEditor(const std::string& navigationId);

        void ActivateEditor(std::string_view navigationId, bool requestFocus);
        void ActivateScene(bool requestFocus);

        void RefreshAssetPanelsVisibility();
        void RemoveAllEditors();

        void OnLayoutChanged(Gui::EditorLayoutType layout) noexcept;

        [[nodiscard]] bool HasEditors() const noexcept { return !assetEditors_.empty(); }
        [[nodiscard]] const std::string& GetActiveNavigationId() const noexcept { return activeNavigationId_; }
        [[nodiscard]] Gui::EditorLayoutType GetActiveLayout() const noexcept { return activeLayout_; }
        [[nodiscard]] const std::vector<std::string>& GetEditorOrder() const noexcept { return assetEditorOrder_; }

        struct AssetEditorEntry;
        [[nodiscard]] AssetEditorEntry* FindEditor(std::string_view navigationId) noexcept;

    private:
        struct PanelSet
        {
            Gui::GuiPanel* toolbar{nullptr};
            Gui::GuiPanel* viewport{nullptr};
            Gui::GuiPanel* outline{nullptr};
            Gui::GuiPanel* inspector{nullptr};

            template<typename Fn>
            void ForEachPanel(Fn&& fn) const
            {
                if (toolbar) std::forward<Fn>(fn)(toolbar);
                if (viewport) std::forward<Fn>(fn)(viewport);
                if (outline) std::forward<Fn>(fn)(outline);
                if (inspector) std::forward<Fn>(fn)(inspector);
            }

            [[nodiscard]] std::span<Gui::GuiPanel*> CopyTo(std::span<Gui::GuiPanel*> buffer) const noexcept;
        };

        struct AssetMetadata
        {
            std::string extension{};
            std::string assetType{};
        };

    public:
        struct AssetEditorEntry
        {
            std::filesystem::path assetPath;
            std::string navigationId;
            std::string buttonLabel;
            AssetMetadata metadata;
            PanelSet panels{};
            std::shared_ptr<Gui::BaseAssetEditorController::SharedState> sharedState;
        };

    private:
        struct PathHash
        {
            std::size_t operator()(const std::filesystem::path& value) const noexcept
            {
                return std::hash<std::string>{}(value.generic_string());
            }
        };

        using PanelBuffer = std::array<Gui::GuiPanel*, 4>;

        void SwitchToLayout(Gui::EditorLayoutType layout, std::string_view navId, Gui::GuiPanel* panelToFocus = nullptr);
        void ApplyPanels(AssetEditorEntry& entry);
        [[nodiscard]] std::span<Gui::GuiPanel*> CollectPanels(const PanelSet& panels, PanelBuffer& buffer) const noexcept;
        void DetachAndRemovePanels(const PanelSet& panels);
        void FocusPanel(Gui::GuiPanel* panel) const;
        void RequestSceneFocus() const;

        bool CreateAssetEditorEntry(const std::filesystem::path& path, AssetEditorEntry& outEntry);
        bool CreateActorPrefabEditor(const std::filesystem::path& path, AssetEditorEntry& outEntry, const std::string& navigationId);
        bool CreateComponentPrefabEditor(const std::filesystem::path& path, AssetEditorEntry& outEntry, const std::string& navigationId);
        std::shared_ptr<Gui::BaseAssetEditorController::SharedState> BuildSharedState(const std::filesystem::path& path, String stableIdRoot, const std::string& assetType, std::function<void()> onCloseRequest);
        void PopulatePrefabMetadata(const std::filesystem::path& path, Gui::BaseAssetEditorController::SharedState& state);
        std::string MakeNavigationId(const std::filesystem::path& path, std::string_view typeTag) const;

        Gui::GuiManager* guiManager_{nullptr};
        Gui::GuiLayoutManager* layoutManager_{nullptr};
        FocusRequestCallback focusRequestCallback_{};
        FocusSceneCallback focusSceneCallback_{};

        std::unordered_map<std::string, AssetEditorEntry> assetEditors_{};
        std::unordered_map<std::filesystem::path, std::string, PathHash> editorsByPath_{};
        std::vector<std::string> assetEditorOrder_{};

        std::string activeNavigationId_{"scene"};
        Gui::EditorLayoutType activeLayout_{Gui::EditorLayoutType::Scene};
    };
}
