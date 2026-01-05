#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include "Containers/String.h"
#include "Gui/Controllers/GuiPanelWindow.h"
#include "imgui.h"


namespace BixEngine::Gui
{
    class BaseAssetEditorWindow : public GuiPanelWindow
    {
    public:
        
        // --- 1. State ---
        
        struct SharedState
        {
            struct VariableMetadata
            {
                String name;
                String type;
                String value;
            };

            std::filesystem::path assetPath{};
            String assetDisplayName{};
            String assetTypeLabel{"Asset"}; // Ex: "Texture", "Sound", "Prefab"
            
            String stableIdRoot{}; 
            
            bool isDirty{false}; 
            std::function<void()> onCloseRequest{};
            
            // Metadata for Prefabs/Scripted Objects
            std::vector<VariableMetadata> exposedVariables;
            String primaryClassName{};
            String includePath{};
        };

        struct PanelConfig
        {
            String titlePrefix{};
            DockSpaceRegion dockRegion{DockSpaceRegion::Center};
            ImGuiCond dockCondition{ImGuiCond_FirstUseEver};
            String stableIdSuffix{};
        };

        BaseAssetEditorWindow(std::shared_ptr<SharedState> sharedState, PanelConfig config);
        ~BaseAssetEditorWindow() override = default;

        [[nodiscard]] std::shared_ptr<SharedState> GetSharedState() const noexcept { return state_; }

    protected:
        // --- 2. Outils pour les enfants ---
        
        virtual void DrawStandardToolbar();
        virtual void DrawToolbarExtensions() {}

        virtual void DrawPanelContents(GuiPanel& panel) = 0;

        virtual void OnSaveRequested();
        
        virtual void OnPlayRequested() {}
        virtual void OnCompileRequested() {}

        [[nodiscard]] const PanelConfig& GetPanelConfig() const noexcept { return config_; }

    private:
        void OnAttach(GuiPanel& panel) override;
        void OnDetach(GuiPanel& panel) override;
        void OnDraw(GuiPanel& panel) override;

        void ApplyPanelTitle(GuiPanel& panel);

        PanelConfig config_{};
        std::shared_ptr<SharedState> state_{};
        
        String cachedDisplayName_{};
        bool cachedDirtyState_{false}; 
        String stablePanelId_{};
    };
}