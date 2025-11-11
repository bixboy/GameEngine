#pragma once
#include <filesystem>
#include "Engine/Gui/Dialogs/ModalDialog.h"

namespace BixEngine::Gui
{
    class CreateFolderDialog : public ModalDialog
    {
        
    public:
        CreateFolderDialog(ContentBrowserState& state, String& selectedEntry);
        
        void Open(const path& targetDirectory);

    protected:
        void DrawContent() override;

    private:
        void DrawHeader();
        
        void DrawInputField();
        
        void DrawError();
        
        void DrawFooter();

        bool TryCreate();

        char folderName_[128];
        String folderError_;
        path targetDir_;
    };
}
