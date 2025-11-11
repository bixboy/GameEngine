#pragma once

#include <filesystem>
#include "Core/Containers/String.h"
#include "Engine/Gui/Dialogs/ModalDialog.h"

namespace BixEngine::Gui
{
    class CreateSpriteAtlasDialog : public ModalDialog
    {
    public:
        CreateSpriteAtlasDialog(ContentBrowserState& state, String& selectedEntry);

        void Open(const std::filesystem::path& sourcePath);

    protected:
        void DrawContent() override;

    private:
        
        // UI rendering
        void DrawHeader();
        void DrawInputFields();

        bool TryGenerateAtlas();

        void RenameGeneratedAtlasIfNeeded(const std::string& desiredBaseName);

        char atlasName_[128];

        int columns_;
        int rows_;
        float frameRate_;
        bool loop_;

        std::filesystem::path framesDir_;
        String atlasError_;
    };
}
