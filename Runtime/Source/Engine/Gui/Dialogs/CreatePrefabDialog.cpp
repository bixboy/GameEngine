#include "Engine/Gui/Dialogs/CreatePrefabDialog.h"
#include "Engine/Gui/Dialogs/CreateScriptDialog.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserState.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

#include "Engine/Utils/PrefabUtils.h"

using namespace BixEngine::Gui;
using namespace BixEngine::Gui::Utils;


// ============================================================================
// CLASS IMPLEMENTATION
// ============================================================================
CreatePrefabDialog::CreatePrefabDialog(ContentBrowserState& state, String& selectedEntry)
: ModalDialog(state, selectedEntry, "ContentBrowserCreatePrefab")
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
    selectedIsActor_ = false;
    selectedIsComponent_ = false;
}

void CreatePrefabDialog::SetSelectedScript(const std::string& className, const std::string& includePath,
                                           const std::string& assetBaseName, bool isActor, bool isComponent)
{
    selectedClass_ = className.c_str();
    selectedInclude_ = includePath.c_str();
    selectedAssetBaseName_ = assetBaseName.c_str();
    selectedIsActor_ = isActor || (!isComponent);
    selectedIsComponent_ = isComponent;
}

// ============================================================================
// UI RENDERING
// ============================================================================
void CreatePrefabDialog::DrawContent()
{
    EnsureScriptsDirectoryExists(state_);

    const path scriptsDir = state_.root / "Scripts";
    auto scripts = ScriptUtils::BuildScriptTree(scriptsDir, state_.root);
    auto bases = PrefabUtils::GetBaseClasses();
    
    std::vector<PrefabUtils::PrefabScriptCandidate> candidates = PrefabUtils::GatherPrefabCandidates(scripts, bases);

    DrawDescriptionText("Create a prefab asset linked to an existing gameplay script.");
    ImGui::Spacing();

    DrawSearchBar();
    std::string filter = searchBuffer_;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

    DrawCandidateListUI(candidates, filter);
    
    if (!prefabError_.IsEmpty())
        DrawErrorMessage(std::string(prefabError_.View()));

    DrawDetailsSectionUI();
    DrawActionButtons();
}

void CreatePrefabDialog::DrawSearchBar()
{
    ImGui::InputTextWithHint("##PrefabSearch", "Search scripts...", searchBuffer_, IM_ARRAYSIZE(searchBuffer_));
}

void CreatePrefabDialog::DrawCandidateListUI(const std::vector<PrefabUtils::PrefabScriptCandidate>& candidates, const std::string& filter)
{
    float listHeight = ImGui::GetTextLineHeightWithSpacing() * 12.0f;
    if (!ImGui::BeginChild("PrefabCandidateList", ImVec2(420, listHeight), true))
        return;

    if (candidates.empty())
    {
        ImGui::TextDisabled("No eligible scripts were found.");
        ImGui::EndChild();
        
        return;
    }

    for (const auto& candidate : candidates)
    {
        std::string nameLower = candidate.displayName;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), tolower);
        
        if (!filter.empty() && nameLower.find(filter) == std::string::npos)
            continue;

        std::string label = candidate.displayName;
        
        if (candidate.isActor && candidate.isComponent)
            label += " [Actor/Component]";
        
        else if (candidate.isActor)
            label += " [Actor]";
        
        else if (candidate.isComponent)
            label += " [Component]";
        
        else if (candidate.hasBlueprintMacro)
            label += " [Blueprint]";

        bool isSelected = (!selectedClass_.IsEmpty() && selectedClass_.View() == candidate.className);
        if (ImGui::Selectable(label.c_str(), isSelected))
        {
            SetSelectedScript(candidate.className, candidate.includePath, candidate.assetBaseName, candidate.isActor, candidate.isComponent);
            prefabError_.Clear();
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
        {
            ImGui::BeginTooltip();
            ImGui::Text("Class: %s", candidate.className.c_str());
            
            if (!candidate.includePath.empty())
                ImGui::Text("Include: %s", candidate.includePath.c_str());
            
            if (!candidate.headerPath.empty())
                ImGui::Text("Header: %s", candidate.headerPath.generic_string().c_str());
            
            ImGui::EndTooltip();
        }
    }

    ImGui::EndChild();
}

void CreatePrefabDialog::DrawDetailsSectionUI()
{
    DrawSeparatorText("Prefab details");

    bool makeComponent = selectedIsComponent_ && !selectedIsActor_;
    const char* typeLabel = makeComponent ? "Component" : "Actor";
    const char* ext = makeComponent ? ".bixcomponent" : ".bixactor";

    DrawLabelValue("Script", selectedClass_.IsEmpty() ? "None" : selectedClass_.View().data(), "None");
    DrawLabelValue("Type", typeLabel, "Actor");

    std::string baseName = selectedAssetBaseName_.IsEmpty()
        ? (selectedClass_.IsEmpty() ? "Prefab" : PrefabUtils::SanitizeAssetName(selectedClass_.View().data()))
        : selectedAssetBaseName_.ToStdString();

    path relDir = state_.current.lexically_relative(state_.root);
    
    std::string location = relDir.empty() || relDir.generic_string() == "." ? "Content" : "Content/" + relDir.generic_string();

    DrawLabelValue("Location", location, "Content");
    DrawLabelValue("File", baseName + ext, "Prefab.bixactor");
}

bool CreatePrefabDialog::DrawActionButtons()
{
    bool confirm = DrawConfirmButtons("Create", "Cancel", [] {},
        [&]
        {
            Close();
            ClearSelection();
            prefabError_.Clear();
        });

    if (!confirm)
        return false;

    if (selectedClass_.IsEmpty())
        return LogAndStoreError(prefabError_, "Please select a script to instantiate.", false);

    bool makeComponent = selectedIsComponent_ && !selectedIsActor_;
    const char* ext = makeComponent ? ".bixcomponent" : ".bixactor";
    const char* type = makeComponent ? "Component" : "Actor";

    std::string baseName = selectedAssetBaseName_.IsEmpty()
        ? selectedClass_.View().data()
        : selectedAssetBaseName_.View().data();

    path target = state_.current / (baseName + ext);
    if (fs::exists(target))
        return LogAndStoreError(prefabError_, "An asset with this name already exists.", false);

    if (!PrefabUtils::ValidateMetadata(selectedClass_.ToStdString(), selectedInclude_.ToStdString(), prefabError_))
        return false;

    std::ostringstream json;
    json << "{\n"
         << "  \"type\": \"" << type << "\",\n"
         << "  \"class\": \"" << PrefabUtils::EscapeJson(selectedClass_.ToStdString()) << "\"";

    if (!selectedInclude_.IsEmpty())
        json << ",\n  \"include\": \"" << PrefabUtils::EscapeJson(selectedInclude_.ToStdString()) << "\"";

    json << "\n}\n";

    if (TryWriteFile(target, json.str(), prefabError_))
    {
        Close();
        ClearSelection();
        prefabError_.Clear();
        state_.cache.dirty = true;
    }
}
