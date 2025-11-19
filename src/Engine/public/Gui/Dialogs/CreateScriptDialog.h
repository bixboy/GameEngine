#pragma once
#include "Gui/Dialogs/ModalDialog.h"
#include "Utils/ScriptUtils.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"

namespace BixEngine::Gui
{
    class CreateScriptDialog final : public ModalDialog
    {
    public:
        CreateScriptDialog(ContentBrowserState& state, String& selectedEntry);

        void Open() override;

    private:
        void DrawContent() override;

        void ClearParentSelection();
        void SetSelectedParent(const BixEngine::ScriptUtils::ParentScriptInfo& info, bool isBase);

        char scriptName_[128]{};
        String scriptError_{};

        String selectedParentClass_{};
        String selectedParentInclude_{};
        String selectedParentDisplay_{};
        bool selectedParentIsBase_{false};
        bool selectedParentIsActor_{false};
        bool selectedParentIsComponent_{false};

        ScriptTemplateType scriptType_{ScriptTemplateType::Actor};
    };
}
