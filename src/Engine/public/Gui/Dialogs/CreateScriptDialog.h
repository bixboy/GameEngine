#pragma once
#include <filesystem>
#include "Containers/String.h"
#include "Gui/Dialogs/ModalDialog.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"

namespace BixEngine::Gui
{
    class CreateScriptDialog : public ModalDialog
    {
    public:
        CreateScriptDialog(ContentBrowserState& state, String& selectedEntry);

        void Open() override;

    protected:
        void DrawContent() override;

    private:
        char scriptName_[128];

        String selectedParentClass_;
        String selectedParentInclude_;
        String selectedParentDisplay_;

        bool selectedParentIsBase_;
        bool selectedParentIsActor_;
        bool selectedParentIsComponent_;

        ContentBrowserState* statePtr_;

        enum class ScriptTemplateType { Actor = 0, Component, Utility };

        ScriptTemplateType scriptType_;

        String scriptError_;

        void ClearParentSelection();
        void SetSelectedParent(const std::string& className, const std::string& includePath, bool isActor,
                               bool isComponent, bool isBase);
    };
}
