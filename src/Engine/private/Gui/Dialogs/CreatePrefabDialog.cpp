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
#include "Debug/Logger.h"

using namespace BixEngine::Gui;
using namespace BixEngine::Gui::GuiUtils;
using namespace BixEngine::Utils;
namespace fs = std::filesystem;

CreatePrefabDialog::CreatePrefabDialog(ContentBrowserState& state, String& selectedEntry) : ModalDialog(state, selectedEntry, "ContentBrowserCreatePrefab")
{
    ClearSelection();
}

void CreatePrefabDialog::Open()
{
    prefabError_.clear();
    ClearSelection();
    ModalDialog::Open();
}

void CreatePrefabDialog::ClearSelection()
{
    selectedClass_.clear();
    selectedInclude_.clear();
    selectedAssetBaseName_.clear();
    selectedHeaderPath_.clear();
    std::snprintf(assetNameBuffer_, sizeof(assetNameBuffer_), "%s", "Prefab");
    selectedIsActor_ = false;
    selectedIsComponent_ = false;
}

void CreatePrefabDialog::DrawContent()
{
    // Minimal implementation as original was corrupted
    DrawDescriptionText("Create a new Prefab from a C++ class.");
    
    // We would expect UI to select a class here. 
    // For now, leaving empty to allow compilation.
}

bool CreatePrefabDialog::TryCreatePrefab()
{
    if (selectedClass_.IsEmpty())
    {
         FileUtils::LogAndStoreError(prefabError_, "No class selected.", false);
         return false;
    }
    
    String baseName = String(assetNameBuffer_);
    if (baseName.IsEmpty())
    {
         FileUtils::LogAndStoreError(prefabError_, "Prefab name cannot be empty.", false);
         return false;
    }

    path target = state_.current / (baseName.Std() + ".prefab");
    if (fs::exists(target))
    {
         FileUtils::LogAndStoreError(prefabError_, "File already exists.", false);
         return false;
    }

    if (!BixEngine::Editor::ScriptIntrospector::ValidateMetadata(selectedClass_.Std(), selectedInclude_.Std(), prefabError_))
        return false;

    auto newActor = BixEngine::Serialization::SceneSerializer::CreateActor(selectedClass_.Std().c_str());
    
    if (!newActor)
    {
        return FileUtils::LogAndStoreError(prefabError_, "Failed to instantiate actor class: " + selectedClass_.Std(), false);
    }

    newActor->SetName(baseName);

    if (BixEngine::Serialization::PrefabSerializer::SavePrefab(newActor.get(), target))
    {
        Close();
        ClearSelection();
        prefabError_.clear();
        state_.cache.dirty = true;
        return true;
    }
    
    return false;
}
