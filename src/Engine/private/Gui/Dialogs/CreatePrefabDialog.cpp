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
#include "Utils/Editor/ScriptUtils.h"
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
    DrawDescriptionText("Create a new Prefab from a C++ class.");
    
    // 1. Name Input
    InputTextWithLabel("Prefab Name", assetNameBuffer_, sizeof(assetNameBuffer_));

    ImGui::Separator();

    // 2. Class Selection
    static ImGuiTextFilter filter;
    filter.Draw("Search Class");

    std::string selectedDisplayName = selectedClass_.IsEmpty() ? "None" : selectedClass_.Std();
    ImGui::Text("Selected Class: %s", selectedDisplayName.c_str());

    ImGui::BeginChild("ClassList", ImVec2(0, 200), true);
    
    // We need a way to list available Actor classes. 
    // Assuming ScriptIntrospector has them or we scan. 
    // For now, let's look for "BixEngine::Editor::ScriptIntrospector::GetActorClasses()" or similar.
    // If not available, we can't implement full selection. 
    // But since this file was likely corrupted/wiped, I should check how it was getting classes.
    // The includes suggested ScriptIntrospector.
    
    // Gather candidates
    path contentRoot = ContentBrowserUtils::GetContentRoot();
    path scriptRoot = contentRoot.parent_path() / "src";
    if(!fs::exists(scriptRoot))
        scriptRoot = contentRoot.parent_path() / "Source";

    // Fallback if no src/Source found
    if(!fs::exists(scriptRoot))
         scriptRoot = contentRoot; 

    // Re-use static cache if possible or just rebuild for now (dialog isn't perf critical per frame if done once or guarded)
    // But DrawContent is called every frame! We cannot scan recursive directories every frame.
    // We must use a static or member cache.
    // Let's make it static since it's inside DrawContent and we don't want to add members to the header if I can avoid touching it (users rules/effort).
    // Actually, I can use a static with a simple dirty check or just scan once when dialog opens?
    // Dialog::Open() clears selection. I could clear cache there if I had access, but I don't want to modify header now if I can avoid.
    // I'll use a static flag and refresh button or just refresh when `filter` is empty? No.
    // Best: Helper function `GetCachedCandidates` with a static 
    
    static std::vector<BixEngine::Editor::ScriptIntrospector::PrefabScriptCandidate> cachedCandidates;
    static bool cachePopulated = false;
    
    // We can populate cache if empty.
    if (!cachePopulated)
    {
         auto roots = BixEngine::ScriptUtils::Utilities::BuildScriptTree(scriptRoot, contentRoot);
         auto baseClasses = BixEngine::Editor::ScriptIntrospector::GetBaseClasses();
         cachedCandidates = BixEngine::Editor::ScriptIntrospector::GatherPrefabCandidates(roots, baseClasses);
         cachePopulated = true;
    }
    
    if (ImGui::Button("Refresh"))
        cachePopulated = false;

    for(const auto& cls : cachedCandidates)
    {
        if (!cls.isActor) continue; 
        
        if(filter.PassFilter(cls.className.c_str()))
        {
            if(ImGui::Selectable(cls.className.c_str(), selectedClass_.Std() == cls.className))
            {
                selectedClass_ = cls.className;
                selectedInclude_ = cls.includePath;
            }
        }
    }

    ImGui::EndChild();
    
    DrawErrorMessage(prefabError_.Std());

    if(DrawConfirmButtons("Create", "Cancel"))
    {
        TryCreatePrefab();
    }
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
