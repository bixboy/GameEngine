#pragma once
#include <filesystem>
#include "Gui/Dialogs/ModalDialog.h"
#include "Containers/String.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Utils/FileIO/PrefabUtils.h"

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
        
        void DrawSearchBar();
        void DrawCandidateListUI(const std::vector<PrefabUtils::Utilities::PrefabScriptCandidate>& candidates, const std::string& filter);
        void DrawDetailsSectionUI();
        bool DrawActionButtons();

        
        void ClearSelection();
        void SetSelectedScript(const std::string& className, const std::string& includePath, const std::string& assetBaseName, bool isActor, bool isComponent, const path& headerPath);

        char searchBuffer_[128]{};
        char assetNameBuffer_[128]{};

        String selectedClass_;
        String selectedInclude_;
        String selectedAssetBaseName_;
        path selectedHeaderPath_{};

        bool selectedIsActor_ = false;
        bool selectedIsComponent_ = false;

        String prefabError_;
    };
}
