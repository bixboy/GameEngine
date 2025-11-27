#pragma once
#include "Gui/Dialogs/ModalDialog.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"

namespace BixEngine::Gui
{
    class CreateAudioContainerDialog final : public ModalDialog
    {
    public:
        CreateAudioContainerDialog(ContentBrowserState& state, String& selectedEntry);

        void Open(const std::filesystem::path& currentPath);

    private:
        void DrawContent() override;

        char nameBuffer_[128]{};
        String error_{};
        std::filesystem::path currentPath_;
    };
}
