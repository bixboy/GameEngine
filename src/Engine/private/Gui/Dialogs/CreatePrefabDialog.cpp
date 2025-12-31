#include "Gui/Dialogs/CreatePrefabDialog.h"
#include "Gui/Dialogs/CreateScriptDialog.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"

#include <algorithm>
#include <cstdio>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <vector>
#include <fstream>

#include "Gui/Utils/ContentBrowserUtils.h"
#include "Utils/FileIO/FilesUtils.h"
#include "Utils/Editor/ScriptIntrospector.h"
#include "Serializer/PrefabSerializer.h"
#include "Serializer/SceneSerializer.h"
#include "Utils/FileIO/BinaryUtils.h"
#include "Framework/Actor.h"

using namespace BixEngine::Gui;
using namespace BixEngine::Gui::Utils;
namespace fs = std::filesystem;





CreatePrefabDialog::CreatePrefabDialog(ContentBrowserState& state, String& selectedEntry) : ModalDialog(state, selectedEntry, "ContentBrowserCreatePrefab")
{
    ClearSelection();
}

void CreatePrefabDialog::Open()
{
    prefabError_.Clear();
    ClearSelection();
    ModalDialog::Open();
}

void CreatePrefabDialog::ClearSelection()
{
    selectedClass_.Clear();
    selectedInclude_.Clear();
    selectedAssetBaseName_.Clear();
    selectedHeaderPath_.clear();
    std::snprintf(assetNameBuffer_, sizeof(assetNameBuffer_), "%s", "Prefab");
    selectedIsActor_ = false;
    selectedIsComponent_ = false;
}

    if (!BixEngine::Editor::ScriptIntrospector::ValidateMetadata(selectedClass_.ToStdString(), selectedInclude_.ToStdString(), prefabError_))
        return false;

    LOG_INFO("CreatePrefabDialog: Creating Actor instance of type: " + selectedClass_.ToStdString());
    
    
    auto newActor = BixEngine::Serialization::SceneSerializer::CreateActor(selectedClass_.ToStdString().c_str());
    
    if (!newActor)
    {
        return FilesUtils::Utilities::LogAndStoreError(prefabError_, "Failed to instantiate actor class: " + selectedClass_.ToStdString(), false);
    }

    newActor->SetName(baseName);
    
    
    LOG_INFO("CreatePrefabDialog: Created actor has " + std::to_string(newActor->GetComponents().size()) + " components.");

    LOG_INFO("CreatePrefabDialog: Serializing directly to Prefab V2...");

    if (BixEngine::Serialization::PrefabSerializer::SavePrefab(newActor.get(), target))
    {
        LOG_INFO("CreatePrefabDialog: File written successfully. Closing.");
        Close();
        ClearSelection();
        prefabError_.Clear();
        state_.cache.dirty = true;
        return true;
    }
    
    LOG_ERROR("CreatePrefabDialog: File write failed.");
    return false;
}
