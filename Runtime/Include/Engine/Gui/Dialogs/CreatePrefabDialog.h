#pragma once
#include <filesystem>
#include "Engine/Gui/Dialogs/ModalDialog.h"
#include "Core/Containers/String.h"
#include "Engine/Utils/PrefabUtils.h"

namespace BixEngine::Gui
{

    class CreatePrefabDialog : public ModalDialog
    {
        
    public:
        CreatePrefabDialog(ContentBrowserState& state, String& selectedEntry);
        
        void Open() override;

    protected:
        void DrawContent() override;

    private:
        
        // UI helpers
        void DrawSearchBar();
        void DrawCandidateListUI(const std::vector<PrefabUtils::PrefabScriptCandidate>& candidates, const std::string& filter);
        void DrawDetailsSectionUI();
        bool DrawActionButtons();

        // Logic helpers
        void ClearSelection();
        void SetSelectedScript(const std::string& className, const std::string& includePath,
                               const std::string& assetBaseName, bool isActor, bool isComponent);

    private:
        char searchBuffer_[128]{};

        String selectedClass_;
        String selectedInclude_;
        String selectedAssetBaseName_;
        
        bool selectedIsActor_ = false;
        bool selectedIsComponent_ = false;

        String prefabError_;
    };
}
