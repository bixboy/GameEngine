#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>
#include "Gui/Controllers/BaseAssetEditorWindow.h"
#include "Render/Sprite/SpriteFrame.h"
#include "Ressources/Atlas/SpriteAtlasUtils.h" // Vérifie tes includes


namespace BixEngine::Gui
{
    class SpriteAtlasEditorWindow final : public BaseAssetEditorWindow
    {
    public:
        struct SharedState : BaseAssetEditorWindow::SharedState
        {
            Resources::SpriteAtlasDefinition definition{};
            std::vector<Resources::SpriteAnimationDefinition> animations{};
            
            // Texture Data
            std::filesystem::path textureAbsolutePath{};
            std::shared_ptr<Resources::Texture> texture{};
            std::vector<Resources::SpriteFrame> frames{};
            
            // Selection & UI State
            std::vector<bool> frameSelection{};
            int lastSelectedFrameIndex{-1};
            bool dirty{false};
            
            // Editor View State
            float mainViewScale{1.0f};
            int activeAnimation{-1};
            
            // Caching layout
            int cachedColumns{-1};
            int cachedRows{-1};
            int cachedPadding{-1};
            int cachedMargin{-1};
            
            // Preview State
            int previewAnimationIndex{-1};
            int previewFrame{0};
            float previewTimer{0.0f};
            float previewScale{2.0f};
            bool previewPlaying{false};
            
            String error{};
        };

        explicit SpriteAtlasEditorWindow(std::shared_ptr<SharedState> sharedState);
        ~SpriteAtlasEditorWindow() override;

        SpriteAtlasEditorWindow(const SpriteAtlasEditorWindow&) = delete;
        SpriteAtlasEditorWindow& operator=(const SpriteAtlasEditorWindow&) = delete;

        SpriteAtlasEditorWindow(SpriteAtlasEditorWindow&&) = delete;
        SpriteAtlasEditorWindow& operator=(SpriteAtlasEditorWindow&&) = delete;
        // ------------------------------------------------

        static std::shared_ptr<BaseAssetEditorWindow::SharedState> CreateSharedState(const std::filesystem::path& atlasPath,
            String stableIdRoot, std::function<void()> onCloseRequest);

    protected:
        void DrawPanelContents(GuiPanel& panel) override;
        void OnSaveRequested() override;

    private:
        // UI Sections
        void DrawAtlasPreview(SharedState& state);
        void DrawAnimationSection(SharedState& state);
        void DrawAnimationPreview(SharedState& state, Resources::SpriteAnimationDefinition& animation);
        void DrawSaveSection(SharedState& state);
        
        // Helpers
        void RefreshTexture(SharedState& state);
        void EnsureFramesGenerated(SharedState& state);
        void EnsureSelectionSize(SharedState& state);
        
        // Logic
        void HandleSelectionInput(SharedState& state, int hoveredIndex);
        void AssignSelectionToAnimation(SharedState& state, Resources::SpriteAnimationDefinition& animation);
        void RemoveSelectedAnimation(SharedState& state, int index);
        bool SaveAtlas(SharedState& state);
        int FrameCount(const SharedState& state) const noexcept;
        
        // Graphics Helpers
        void DrawCheckerboard(ImDrawList* drawList, ImVec2 pos, ImVec2 size, ImU32 col1, ImU32 col2);
    };
}