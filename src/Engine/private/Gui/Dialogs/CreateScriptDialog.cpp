#include "Gui/Dialogs/CreateScriptDialog.h"
#include "Gui/Utils/GuiHelpers.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <unordered_map>
#include <vector>
#include "IO/FileUtils.h"
#include "Gui/Utils/ContentBrowserUtils.h"
#include "Utils/FileIO/PrefabUtils.h"
#include "Utils/FileIO/FilesUtils.h"
#include "Utils/Editor/HeaderGeneratorUtils.h"
#include "Utils/Editor/ScriptUtils.h"

using namespace BixEngine::Gui;
using namespace BixEngine::Gui::Utils;

// ============================================================================
//  CreateScriptDialog
// ============================================================================

CreateScriptDialog::CreateScriptDialog(ContentBrowserState& state, String& selectedEntry)
    : ModalDialog(state, selectedEntry, "ContentBrowserCreateScript"),
      selectedParentIsBase_(false),
      selectedParentIsActor_(false),
      selectedParentIsComponent_(false),
      scriptType_(ScriptTemplateType::Actor)
{
    std::snprintf(scriptName_, sizeof(scriptName_), "%s", "NewScript");
    scriptError_.Clear();
}

void CreateScriptDialog::Open()
{
    std::snprintf(scriptName_, sizeof(scriptName_), "%s", "NewScript");
    scriptError_.Clear();
    
    ClearParentSelection();
    scriptType_ = ScriptTemplateType::Actor;
    
    ModalDialog::Open();
}

void CreateScriptDialog::ClearParentSelection()
{
    selectedParentClass_.Clear();
    selectedParentInclude_.Clear();
    selectedParentDisplay_.Clear();
    selectedParentIsBase_ = false;
    selectedParentIsActor_ = false;
    selectedParentIsComponent_ = false;
}

void CreateScriptDialog::SetSelectedParent(const BixEngine::ScriptUtils::ParentScriptInfo& info, bool isBase)
{
    selectedParentClass_   = info.className.c_str();
    selectedParentInclude_ = info.includePath.c_str();
    if (!info.displayName.empty())
        selectedParentDisplay_ = info.displayName.c_str();
    else
        selectedParentDisplay_ = info.className.c_str();
    selectedParentIsBase_  = isBase;
    selectedParentIsActor_ = info.isActor;
    selectedParentIsComponent_ = info.isComponent;

    if (info.isComponent)
    {
        scriptType_ = ScriptTemplateType::Component;
    }
    else if (info.isActor)
    {
        scriptType_ = ScriptTemplateType::Actor;   
    }
}

void CreateScriptDialog::DrawContent()
{
    ContentBrowserUtils::EnsureScriptsDirectoryExists(state_);

    const path scriptsDirectory = state_.root / "Scripts";
    auto scriptRoots = ScriptUtils::Utilities::BuildScriptTree(scriptsDirectory, state_.root);
    
    std::unordered_map<std::string, BixEngine::ScriptUtils::ParentScriptInfo> scriptInfoMap;
    std::vector<TreeNodeData> scriptTree = ScriptUtils::Utilities::BuildGuiTree(scriptRoots, scriptInfoMap);

    DrawDescriptionText("Create a new C++ script in the current directory.");
    ImGui::Spacing();
    DrawSeparatorText("Script details");

    // Affiche l’emplacement cible
    path relativeLocation = state_.current.lexically_relative(state_.root);
    std::string locationDisplay = "Content";
    
    if (!relativeLocation.empty() && relativeLocation.string() != ".")
    {
        locationDisplay += '/';
        locationDisplay += relativeLocation.generic_string();
    }
    
    DrawLabelValue("Location", locationDisplay, "Content");

    // Saisie du nom du script
    const bool enterPressed = InputTextWithLabel("Script name (.h / .cpp)",
        scriptName_,
        IM_ARRAYSIZE(scriptName_),
        ImGuiInputTextFlags_EnterReturnsTrue,
        ImGui::IsWindowAppearing());

    if (!scriptError_.IsEmpty())
        DrawErrorMessage(scriptError_.ToStdString());

    // Prépare le baseName
    String trimmedInput = scriptName_;
    std::string baseName = trimmedInput.IsEmpty() ? std::string() : trimmedInput.ToStdString();

    if (!baseName.empty())
    {
        const std::string headerExt = ".h";
        const std::string sourceExt = ".cpp";
        
        if (baseName.size() > headerExt.size() && baseName.ends_with(headerExt))
            baseName.erase(baseName.size() - headerExt.size());
        
        if (baseName.size() > sourceExt.size() && baseName.ends_with(sourceExt))
            baseName.erase(baseName.size() - sourceExt.size());
    }

    ImGui::Spacing();
    DrawSeparatorText("Parent (optional)");
    ImGui::TextDisabled("Pick an inheritance target or leave empty for a standalone script.");

    // Sélection du parent
    const float listHeight = ImGui::GetTextLineHeightWithSpacing() * 7.0f;
    
    if (ImGui::BeginTable("ParentSelectionTable", 2,
        ImGuiTableFlags_SizingStretchSame |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersOuter))
    {
        ImGui::TableSetupColumn("Base classes");
        ImGui::TableSetupColumn("Existing scripts");
        ImGui::TableHeadersRow();
        ImGui::TableNextRow();

        // Colonne base classes
        ImGui::TableSetColumnIndex(0);
        if (ImGui::BeginChild("BaseClassList", ImVec2(0.0f, listHeight), true))
        {
            const auto& baseParents = BixEngine::PrefabUtils::Utilities::GetBaseClasses();
            for (const auto& base : baseParents)
            {
                const bool isSelected = selectedParentIsBase_ && !selectedParentClass_.IsEmpty() && selectedParentClass_.View() == base.className;

                if (ImGui::Selectable(base.displayName.c_str(), isSelected))
                {
                    SetSelectedParent(base, true);
                }

                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(base.className.c_str());

                    if (!base.includePath.empty())
                        ImGui::Text("Include: %s", base.includePath.c_str());

                    ImGui::EndTooltip();
                }
            }
        }
        ImGui::EndChild();

        // Colonne scripts existants
        ImGui::TableSetColumnIndex(1);
        if (ImGui::BeginChild("UserScriptList", ImVec2(0.0f, listHeight), true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            std::string currentlySelected = !selectedParentIsBase_ && !selectedParentClass_.IsEmpty()
            ? selectedParentClass_.ToStdString()
            : std::string();

            const std::string prevSelection = currentlySelected;

            DrawScriptHierarchyTree(scriptTree, currentlySelected, "No user scripts detected.");

            if (currentlySelected != prevSelection)
            {
                if (auto itInfo = scriptInfoMap.find(currentlySelected); itInfo != scriptInfoMap.end())
                {
                    const BixEngine::ScriptUtils::ParentScriptInfo& info = itInfo->second;
                    SetSelectedParent(info, false);
                }
            }
        }
        ImGui::EndChild();
        ImGui::EndTable();
    }

    // Résumé du parent sélectionné
    ImGui::Spacing();
    std::string parentLabel = "None";
    
    if (!selectedParentClass_.IsEmpty())
        parentLabel = selectedParentDisplay_.ToStdString();

    DrawLabelValue("Selected", parentLabel, "None");
    ImGui::SameLine();
    
    if (IconButton("x", "Clear parent selection"))
        ClearParentSelection();

    if (!selectedParentClass_.IsEmpty())
    {
        ImGui::TextDisabled("Class: %s", selectedParentClass_.c_str());
        
        if (!selectedParentInclude_.IsEmpty())
            ImGui::TextDisabled("Include: %s", selectedParentInclude_.c_str());
    }
    else
    {
        ImGui::TextDisabled("This script will not inherit from another class.");
    }

    // Preview des fichiers générés
    ImGui::Spacing();
    DrawSeparatorText("Preview");
    if (baseName.empty())
    {
        ImGui::TextDisabled("Enter a script name to preview the generated files.");
    }
    else
    {
        path relDir = state_.current.lexically_relative(state_.root);
        std::string relDirStr = relDir.generic_string();
        
        if (!relDirStr.empty() && relDirStr != "." && relDirStr.back() != '/')
            relDirStr += '/';

        const std::string headerPreview = relDirStr + baseName + ".h";
        const std::string sourcePreview = relDirStr + baseName + ".cpp";

        ImGui::BulletText("Content/%s", headerPreview.c_str());
        ImGui::BulletText("Content/%s", sourcePreview.c_str());

        if (!selectedParentClass_.IsEmpty())
        {
            ImGui::BulletText("Inherits from %s", selectedParentClass_.c_str());   
        }
        else
        {
            ImGui::BulletText("Standalone class");
        }
    }

    // Boutons Create / Cancel
    ImGui::Spacing();
    const bool confirm = DrawConfirmButtons("Create", "Cancel", []{},
        [&]
        {
            Close();
            scriptError_.Clear();
            ClearParentSelection();
        });

    const bool createTriggered = enterPressed || confirm;
    if (!createTriggered)
        return;

    // Validation du nom
    String rawName = String(scriptName_);
    if (rawName.IsEmpty())
    {
        FilesUtils::Utilities::LogAndStoreError(scriptError_, "Script name cannot be empty.", false);
        return;
    }
    
    if (rawName.Contains("/") || rawName.Contains("\\"))
    {
        FilesUtils::Utilities::LogAndStoreError(scriptError_, "Script name cannot contain path separators.", false);
        return;
    }

    std::string baseNameStr = rawName.ToStdString();

    // Retire extensions si saisies
    if (baseNameStr.size() > 2 && baseNameStr.ends_with(".h"))
    {
        baseNameStr.erase(baseNameStr.size() - 2);   
    }
    if (baseNameStr.size() > 4 && baseNameStr.ends_with(".cpp"))
    {
        baseNameStr.erase(baseNameStr.size() - 4);   
    }

    if (baseNameStr.empty())
    {
        FilesUtils::Utilities::LogAndStoreError(scriptError_, "Script name cannot be empty.", false);
        return;
    }

    // Chemins de sortie
    path headerPath = state_.current / (baseNameStr + ".h");
    path sourcePath = state_.current / (baseNameStr + ".cpp");

    if (fs::exists(headerPath) || fs::exists(sourcePath))
    {
        FilesUtils::Utilities::LogAndStoreError(scriptError_, "A file with this name already exists.", false);
        return;
    }

    // S’assure que le dossier existe
    std::error_code dirErr;
    fs::create_directories(headerPath.parent_path(), dirErr);
    if (dirErr)
    {
        String msg = "Unable to create directory: ";
        msg += dirErr.message();
        FilesUtils::Utilities::LogAndStoreError(scriptError_, std::move(msg));
        return;
    }

    // Création des fichiers
    std::ofstream headerFile(headerPath);
    if (!headerFile.is_open())
    {
        FilesUtils::Utilities::LogAndStoreError(scriptError_, "Failed to create the header file.");
        return;
    }

    std::ofstream sourceFile(sourcePath);
    if (!sourceFile.is_open())
    {
        headerFile.close();
        std::error_code remErr;
        fs::remove(headerPath, remErr);
        FilesUtils::Utilities::LogAndStoreError(scriptError_, "Failed to create the source file.");
        
        return;
    }

    // Détermine l’héritage
    const bool hasParent = !selectedParentClass_.IsEmpty();
    const bool hasParentInclude = hasParent && !selectedParentInclude_.IsEmpty();
    const bool defaultComponent = (scriptType_ == ScriptTemplateType::Component);
    bool inheritsComponent = selectedParentIsComponent_ || defaultComponent;
    bool inheritsActor = selectedParentIsActor_;

    // Si le parent n’est ni actor ni component (utility), par défaut Actor (sauf template Utility)
    if (!inheritsActor && !inheritsComponent && scriptType_ != ScriptTemplateType::Utility)
        inheritsActor = true;

    const std::string baseType = hasParent ? selectedParentClass_.ToStdString() : (inheritsComponent ? "BixEngine::Game::Component" : "BixEngine::Game::Actor");

    const std::string baseInclude = inheritsComponent ? "Game/Components/Component.h" : "Game/Actor.h";

    // Écrit le header
    headerFile << "#pragma once\n\n";
    headerFile << "// Parent: " << (hasParent ? selectedParentClass_.c_str() : "(none)") << "\n";
    headerFile << "// Created automatically from the Content Browser\n\n";
    headerFile << "#include \"" << baseInclude << "\"\n";
    if (hasParentInclude)
        headerFile << "#include \"" << selectedParentInclude_.c_str() << "\"\n";
    headerFile << "#include \"" << baseNameStr << ".generated.h\"\n\n";
    headerFile << "namespace BixEngine::Game {\n\n";
    headerFile << "    BCLASS()\n";
    headerFile << "    class " << baseNameStr << " : public "
               << (hasParent ? selectedParentClass_.c_str()
                             : (inheritsComponent ? "::BixEngine::Game::Component" : "::BixEngine::Game::Actor"))
               << "\n";
    headerFile << "    {\n";
    headerFile << "        GENERATED_BODY()\n\n";
    headerFile << "    public:\n";
    headerFile << "        using Super = "
               << (hasParent ? selectedParentClass_.c_str()
                             : (inheritsComponent ? "::BixEngine::Game::Component" : "::BixEngine::Game::Actor"))
               << ";\n\n";
    if (inheritsComponent)
        headerFile << "        explicit " << baseNameStr << "(Actor* owner);\n";
    else
        headerFile << "        " << baseNameStr << "();\n";

    headerFile << "\n";
    headerFile << "        void BeginPlay() override;\n";
    headerFile << "        void Update(float deltaTime) override;\n";
    headerFile << "    };\n";
    headerFile << "}\n\n";

    sourceFile << "#include \"" << baseNameStr << ".h\"\n\n";
    sourceFile << "namespace BixEngine::Game {\n\n";
    if (inheritsComponent)
    {
        sourceFile << "    " << baseNameStr << "::" << baseNameStr << "(Actor* owner)\n";
        sourceFile << "        : " << (hasParent ? selectedParentClass_.c_str() : "::BixEngine::Game::Component") << "(owner)\n";
        sourceFile << "    {\n";
        sourceFile << "    }\n\n";
    }
    else
    {
        sourceFile << "    " << baseNameStr << "::" << baseNameStr << "() = default;\n\n";
    }
    sourceFile << "    void " << baseNameStr << "::BeginPlay() {\n";
    sourceFile << "        Super::BeginPlay();\n";
    sourceFile << "    }\n\n";
    sourceFile << "    void " << baseNameStr << "::Update(float deltaTime) {\n";
    sourceFile << "        Super::Update(deltaTime);\n";
    sourceFile << "    }\n";
    sourceFile << "}\n\n";

    headerFile.flush();
    sourceFile.flush();
    
    const bool headerGood = headerFile.good();
    const bool sourceGood = sourceFile.good();
    
    headerFile.close();
    sourceFile.close();

    if (!headerGood || !sourceGood)
    {
        std::error_code remErr1, remErr2;
        fs::remove(headerPath, remErr1);
        fs::remove(sourcePath, remErr2);
        
        FilesUtils::Utilities::LogAndStoreError(scriptError_, "Failed to write the script files to disk.");
        
        return;
    }

    const path selectionKey = headerPath.parent_path() / baseNameStr;
    selectedEntry_ = selectionKey.generic_string().c_str();
    scriptError_.Clear();
    
    ClearParentSelection();
    state_.cache.dirty = true;
    
    Close();

    path toolPath = Core::FindToolExecutable("BixHeaderTool.exe");
    path headerPathAbs = fs::weakly_canonical(headerPath);
    HeaderGeneratorUtils::RunBixHeaderTool(toolPath, headerPathAbs);
}
