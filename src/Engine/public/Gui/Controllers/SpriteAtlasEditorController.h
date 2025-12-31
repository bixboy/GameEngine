#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>
#include "Gui/Controllers/BaseAssetEditorController.h"
#include "Render/Sprite/SpriteFrame.h"
#include "Ressources/Atlas/SpriteAtlasUtils.h"


namespace BixEngine::Gui
{
    class SpriteAtlasEditorController final : public BaseAssetEditorController
    {
    public:
        struct SharedState : BaseAssetEditorController::SharedState
        {
            Resources::SpriteAtlasDefinition definition{};
            std::vector<Resources::SpriteAnimationDefinition> animations{};
            std::filesystem::path atlasPath{};
            std::filesystem::path textureAbsolutePath{};
            std::shared_ptr<Resources::Texture> texture{};
            std::vector<Resources::SpriteFrame> frames{};
            std::vector<bool> frameSelection{};
            bool dirty{false};
            int hoveredFrame{-1};
            int activeAnimation{-1};
            float cachedColumns{-1.0f};
            float cachedRows{-1.0f};
            int cachedPadding{-1};
            int cachedMargin{-1};
            int previewAnimationIndex{-1};
            int previewFrame{0};
            float previewTimer{0.0f};
            float previewScale{2.0f};
            bool previewPlaying{false};
            String error{};
            char renameBuffer[128] = "";
        };

        explicit SpriteAtlasEditorController(std::shared_ptr<SharedState> sharedState);

        static std::shared_ptr<SharedState> CreateSharedState(const std::filesystem::path& atlasPath, String stableIdRoot, std::function<void()> onCloseRequest);

    protected:
        void DrawPanelContents(GuiPanel& panel) override;
        void OnSaveRequested() override;

    private:
        void DrawAtlasPreview(SharedState& state);
        void DrawAnimationSection(SharedState& state);
        void DrawAnimationPreview(SharedState& state, Resources::SpriteAnimationDefinition& animation);
        void DrawSaveSection(SharedState& state);
        void RefreshTexture(SharedState& state);
        void EnsureFramesGenerated(SharedState& state);
        void EnsureSelectionSize(SharedState& state);
        void ToggleFrameSelection(SharedState& state, int frameIndex, bool appendToSelection);
        void AssignSelectionToAnimation(SharedState& state, Resources::SpriteAnimationDefinition& animation);
        [[nodiscard]] bool SaveAtlas(SharedState& state);
        [[nodiscard]] int FrameCount(const SharedState& state) const noexcept;
        void RemoveSelectedAnimation(SharedState& state, int index);
    };
}
