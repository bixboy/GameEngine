#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include "Containers/String.h"
#include "Gui/Dialogs/ModalDialog.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"

namespace BixEngine::Gui
{
    class CreateSpriteAtlasDialog : public ModalDialog
    {
    public:
        using ModalDialog::Open;
        
        CreateSpriteAtlasDialog(ContentBrowserState& state, String& selectedEntry);
        
        void Open(const path& sourcePath);

    protected:
        void DrawContent() override;

    private:
        // UI rendering
        void DrawHeader();
        void DrawInputFields();
        void DrawTextureSelector();

        bool TryGenerateAtlas();
        
        char atlasName_[128];

        int columns_;
        int rows_;
        float frameRate_;
        bool loop_;

        int padding_;
        int margin_;

        path framesDir_;
        path texturePath_;
        std::vector<path> textureCandidates_;

        char texturePathBuffer_[260];

        String atlasError_;

        void RefreshTextureCandidates();
        void SetTexturePath(const path& newPath);
        [[nodiscard]] path ResolveTexturePath() const;
        [[nodiscard]] std::string GetDisplayName(const path& value) const;
        void TryAutoConfigureFromTexture();
    };
}
