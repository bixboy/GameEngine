#pragma once
#include <filesystem>
#include "Containers/String.h"
#include "Gui/Dialogs/ModalDialog.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"


namespace BixEngine::Gui
{
    class CreateSpriteAtlasDialog : public ModalDialog
    {
    public:
        CreateSpriteAtlasDialog(ContentBrowserState& state, String& selectedEntry);

        void Open(const path& sourcePath);

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

        path framesDir_;
        String atlasError_;
    };
}
