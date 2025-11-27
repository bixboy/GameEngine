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

#include "Containers/String.h"
#include "Gui/Controllers/BaseAssetEditorController.h"
#include "Gui/Internal/GuiLayoutManager.h"

namespace BixEngine::Gui
{
    class GuiManager;
    class GuiPanel;
}

namespace BixEngine::Core
{
    using namespace BixEngine::Gui;
    
    class GuiAssetEditorManager
    {
    public:
        using FocusRequestCallback = std::function<void(const std::string&)>;
        using FocusSceneCallback = std::function<void()>;

        GuiAssetEditorManager(GuiManager& guiManager, GuiLayoutManager* layoutManager, FocusRequestCallback focusRequestCallback, FocusSceneCallback focusSceneCallback);

        void SetLayoutManager(GuiLayoutManager* layoutManager) noexcept;
        void SetFocusCallbacks(FocusRequestCallback focusRequestCallback, FocusSceneCallback focusSceneCallback);

        void OpenAssetEditor(const std::filesystem::path& path);
        void CloseAssetEditor(const std::string& navigationId);

        void ActivateEditor(std::string_view navigationId, bool requestFocus);
        void ActivateScene(bool requestFocus);

        void RefreshAssetPanelsVisibility();
        void RemoveAllEditors();

        void OnLayoutChanged(EditorLayoutType layout) noexcept;

        [[nodiscard]] bool HasEditors() const noexcept { return !assetEditors_.empty(); }
        [[nodiscard]] const std::string& GetActiveNavigationId() const noexcept { return activeNavigationId_; }
        [[nodiscard]] EditorLayoutType GetActiveLayout() const noexcept { return activeLayout_; }
        [[nodiscard]] const std::vector<std::string>& GetEditorOrder() const noexcept { return assetEditorOrder_; }

        struct AssetEditorEntry;
        [[nodiscard]] AssetEditorEntry* FindEditor(std::string_view navigationId) noexcept;

    private:
        struct PanelSet
        {
            GuiPanel* toolbar{nullptr};
            GuiPanel* viewport{nullptr};
            GuiPanel* outline{nullptr};
            GuiPanel* inspector{nullptr};

            template <typename Fn>
            void ForEachPanel(Fn&& fn) const
            {
                if (toolbar) std::forward<Fn>(fn)(toolbar);
                if (viewport) std::forward<Fn>(fn)(viewport);
                if (outline) std::forward<Fn>(fn)(outline);
                if (inspector) std::forward<Fn>(fn)(inspector);
            }

            [[nodiscard]] std::span<GuiPanel*> CopyTo(std::span<GuiPanel*> buffer) const noexcept;
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
            std::shared_ptr<BaseAssetEditorController::SharedState> sharedState;
        };

    private:
        struct PathHash
        {
            std::size_t operator()(const std::filesystem::path& value) const noexcept
            {
                return std::hash<std::string>{}(value.generic_string());
            }
        };

        using PanelBuffer = std::array<GuiPanel*, 4>;

        void SwitchToLayout(EditorLayoutType layout, std::string_view navId, GuiPanel* panelToFocus = nullptr);
        void ApplyPanels(AssetEditorEntry& entry);
        
        [[nodiscard]] std::span<GuiPanel*> CollectPanels(const PanelSet& panels, PanelBuffer& buffer) const noexcept;
        
        void DetachAndRemovePanels(const PanelSet& panels);
        void FocusPanel(GuiPanel* panel) const;
        void RequestSceneFocus() const;

        bool CreateAssetEditorEntry(const std::filesystem::path& path, AssetEditorEntry& outEntry);
        bool CreateActorPrefabEditor(const std::filesystem::path& path, AssetEditorEntry& outEntry, const std::string& navigationId);
        bool CreateComponentPrefabEditor(const std::filesystem::path& path, AssetEditorEntry& outEntry, const std::string& navigationId);
        bool CreateSpriteAtlasEditor(const std::filesystem::path& path, AssetEditorEntry& outEntry, const String& navigationId);
        bool CreateAudioContainerEditor(const std::filesystem::path& path, AssetEditorEntry& outEntry, const String& navigationId);
        
        void PopulatePrefabMetadata(const std::filesystem::path& path, BaseAssetEditorController::SharedState& state);
        
        std::string MakeNavigationId(const std::filesystem::path& path, std::string_view typeTag) const;

        GuiManager* guiManager_{nullptr};
        GuiLayoutManager* layoutManager_{nullptr};
        FocusRequestCallback focusRequestCallback_{};
        FocusSceneCallback focusSceneCallback_{};

        std::unordered_map<std::string, AssetEditorEntry> assetEditors_{};
        std::unordered_map<std::filesystem::path, std::string, PathHash> editorsByPath_{};
        std::vector<std::string> assetEditorOrder_{};

        std::string activeNavigationId_{"scene"};
        EditorLayoutType activeLayout_{EditorLayoutType::Scene};
    };
}
