#pragma once
#include <filesystem>
#include "Engine/Gui/Dialogs/ModalDialog.h"

namespace BixEngine::Gui
{
    class RenameEntryDialog : public ModalDialog
    {
        
    public:
        RenameEntryDialog(ContentBrowserState& state, String& selectedEntry);
        
        void Open(const path& targetPath, const path& secondaryPath = path(), bool isScriptGroup = false);

    protected:
        void DrawContent() override;

    private:
        bool HandleScriptRename(const String& newNameStr);
        
        bool HandleSingleRename(const String& newNameStr);
        
        void ResetState();

    private:
        char renameBuffer_[256];
        
        String renameError_;
        
        path targetPath_;
        path secondaryPath_;
        
        bool isScriptGroup_;
    };
}
