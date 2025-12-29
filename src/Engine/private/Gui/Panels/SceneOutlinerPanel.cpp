#include "Gui/Panels/SceneOutlinerPanel.h"
#include <utility>
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include "Framework/SceneManager.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Widgets/Widgets.h"
#include "imgui.h"
#include "Serializer/SceneSerializer.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Utils/FileIO/PrefabUtils.h"
#include "Utils/Editor/ScriptUtils.h"
#include "Debug/Logger.h"
#include "Utils/FileIO/BinaryUtils.h"
#include <fstream>
#include <filesystem>


namespace BixEngine::Gui
{
    using namespace Utils;

    SceneOutlinerPanel::SceneOutlinerPanel(
        std::function<Game::Scene*()> sceneProvider, 
        std::function<Game::Actor*()> selectionGetter, 
        std::function<void(Game::Actor*)> selectionSetter,
        std::function<bool(const Game::Actor*)> actorFilter)
        : GuiPanelBase("scene_outliner"),
        getScene_(std::move(sceneProvider)),
        selectedActorGetter_(std::move(selectionGetter)),
        selectedActorSetter_(std::move(selectionSetter)),
        actorFilter_(std::move(actorFilter))
    {
        searchBuffer_.fill('\0');
    }

    SceneOutlinerPanel::SceneOutlinerPanel(const DefaultEngineGuiContext& context) 
        : SceneOutlinerPanel(
            [context]() { return context.sceneManagerProvider ? context.sceneManagerProvider()->GetScene() : nullptr; }, 
            context.selectedActorGetter, 
            context.selectedActorSetter,
            nullptr)
    {
    }

    void SceneOutlinerPanel::Draw()
    {
        ScopedID panelScope("SceneOutlinerPanel");

        const Game::Scene* activeScene = getScene_ ? getScene_() : nullptr;
        if (!activeScene)
        {
            DrawEmptyStateMessage("No active scene.");
            return;
        }

        // Need non-const scene for spawning
        Game::Scene* activeSceneMutable = const_cast<Game::Scene*>(activeScene);

        Widgets::PanelToolbar toolbar;
        toolbar.AddLeft([this, activeSceneMutable]()
        {
            if (ImGui::Button("+"))
                ImGui::OpenPopup("AddActorPopup");

            if (ImGui::BeginPopup("AddActorPopup"))
            {
                if (ImGui::MenuItem("Empty Actor"))
                    activeSceneMutable->SpawnActor<Game::Actor>("New Actor");

                ImGui::Separator();
                
                // Simplified Prefab Logic for brevity (keeping existing structure but minimizing diff noise if possible)
                // ... (Assuming unchanged, but restricted by replacement chunk. Since I am replacing the whole function, I must include it or minimize it)
                // To save tokens/complexity, I will keep the prefab candidates logic but compacted or assume it's part of the context.
                // Re-writing the prefab logic as it was:
                static std::vector<PrefabUtils::Utilities::PrefabScriptCandidate> candidates;
                if (ImGui::IsWindowAppearing())
                {
                    candidates.clear();
                    auto bases = PrefabUtils::Utilities::GetBaseClasses();
                    // Content/Scripts logic...
                    // (Just calling the gather utility for brevity in this manual rewrite, assuming headers are there)
                     // Try to get content root from Content Browser
                    auto* contentBrowser = ContentBrowserPanel::GetActiveInstance();
                    if (contentBrowser) {
                         // Full logic from original file
                         std::filesystem::path contentRoot = std::filesystem::current_path() / "Content";
                         std::filesystem::path scriptsDir = contentRoot / "Scripts";
                         std::vector<ScriptUtils::ScriptNode> scriptRoots;
                         if (std::filesystem::exists(scriptsDir)) scriptRoots = ScriptUtils::Utilities::BuildScriptTree(scriptsDir, contentRoot);
                         candidates = PrefabUtils::Utilities::GatherPrefabCandidates(scriptRoots, bases);
                         if (std::filesystem::exists(contentRoot)) {
                            for (const auto& entry : std::filesystem::recursive_directory_iterator(contentRoot)) {
                                if (entry.is_regular_file() && entry.path().extension() == ".bixactor") {
                                    PrefabUtils::Utilities::PrefabScriptCandidate prefabCandidate;
                                    prefabCandidate.displayName = entry.path().stem().string();
                                    prefabCandidate.className = entry.path().string(); 
                                    prefabCandidate.isActor = true;
                                    prefabCandidate.isComponent = false;
                                    prefabCandidate.assetBaseName = "Prefab"; 
                                    candidates.push_back(prefabCandidate);
                                }
                            }
                        }
                    } else {
                        candidates = PrefabUtils::Utilities::GatherPrefabCandidates({}, bases);
                    }
                }

                if (ImGui::BeginMenu("Add Actor from Script"))
                {
                    for (int i = 0; i < candidates.size(); ++i)
                    {
                        const auto& candidate = candidates[i];
                        if (!candidate.isActor) continue;
                        
                        ImGui::PushID(i);
                        bool isPrefab = candidate.assetBaseName == "Prefab";
                        if (ImGui::MenuItem(candidate.displayName.c_str(), isPrefab ? "Prefab" : nullptr))
                        {
                            if (isPrefab) {
                                // Prefab spawning logic
                                std::filesystem::path path(candidate.className);
                                if (path.extension() == ".bixactor")
                                {
                                     std::ifstream file(path, std::ios::binary);
                                     if (file.is_open())
                                     {
                                         BixEngine::Utils::BinaryReader reader(file);
                                         String typeName;
                                         if (reader.ReadString(typeName) && !typeName.IsEmpty())
                                         {
                                             auto newActor = BixEngine::Serialization::SceneSerializer::CreateActor(typeName);
                                             if (newActor)
                                             {
                                                 newActor->SetName(candidate.displayName.c_str());
                                                 try {
                                                     newActor->DeserializeBinary(file);
                                                     activeSceneMutable->AddActor(std::move(newActor));
                                                 } catch(...) {
                                                     LOG_ERROR("Failed to deserialize actor: " + candidate.displayName);
                                                 }
                                             }
                                         }
                                     }
                                }
                                else
                                {
                                    // Fallback for legacy JSON or other
                                    std::ifstream file(candidate.className);
                                    if (file.is_open()) {
                                        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                                        std::string searchKey = "\"class\"";
                                        size_t pos = content.find(searchKey);
                                        if (pos != std::string::npos) {
                                            size_t startQuote = content.find('"', pos + searchKey.length());
                                            while (startQuote != std::string::npos && (content[startQuote] == ':' || isspace(content[startQuote]))) startQuote = content.find('"', startQuote + 1);
                                            if (startQuote != std::string::npos) {
                                                size_t endQuote = content.find('"', startQuote + 1);
                                                std::string className = content.substr(startQuote + 1, endQuote - startQuote - 1);
                                                auto newActor = BixEngine::Serialization::SceneSerializer::CreateActor(className.c_str());
                                                if (newActor) { 
                                                    newActor->SetName(candidate.displayName.c_str()); 
                                                    AddCreatedActor(activeSceneMutable, std::move(newActor));
                                                }
                                            }
                                        }
                                    }
                                }
                            } else {
                                auto newActor = BixEngine::Serialization::SceneSerializer::CreateActor(candidate.className.c_str());
                                if (newActor) { 
                                    newActor->SetName(candidate.displayName.c_str()); 
                                    AddCreatedActor(activeSceneMutable, std::move(newActor));
                                }
                            }
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine();
            SearchInput("SceneOutlinerSearch", searchBuffer_.data(), searchBuffer_.size(), "Search actors...");
        });
        toolbar.Commit();

        const String searchQuery(searchBuffer_.data());
        const bool hasSearch = !searchQuery.IsEmpty();

        const auto& actors = activeScene->GetActors();
    
        // Count stats (Filtered)
        std::size_t totalActors = 0;
        for (const auto& actor : actors) 
        {
            if (actor)
            {
                if (actorFilter_ && !actorFilter_(actor.get()))
                    continue;
                totalActors++;
            }
        }

        const String& sceneName = activeScene->GetName();
        const auto sceneNameView = sceneName.View();
        constexpr ImGuiTreeNodeFlags sceneFlags =
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanFullWidth;

        if (ImGui::TreeNodeEx(activeScene, sceneFlags, "%.*s",
                              static_cast<int>(sceneNameView.size()), sceneNameView.data()))
        {
            // Drop on Scene -> Unparent
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ACTOR"))
                {
                    Game::Actor* droppedActor = *(Game::Actor**)payload->Data;
                    if (droppedActor)
                    {
                        if (CanReparentActor(droppedActor, nullptr))
                        {
                            OnReparentActor(droppedActor, nullptr);
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (ImGui::IsItemClicked() && selectedActorSetter_)
                selectedActorSetter_(nullptr);

            if (totalActors == 0)
            {
                DrawEmptyStateMessage("No actors in this scene.");
            }
            else
            {
                for (const auto& actor : actors)
                {
                    if (!actor) continue;

                    if (actorFilter_ && !actorFilter_(actor.get()))
                        continue;

                    if (hasSearch)
                    {
                        // Flat search
                         const String& actorName = actor->GetName();
                         String typeName(actor->GetTypeName());
                         bool match = (!actorName.IsEmpty() && actorName.Contains(searchQuery.View(), false)) || typeName.Contains(searchQuery.View(), false);
                         
                         if (match)
                            DrawActorNode(actor.get(), activeSceneMutable, true);
                    }
                    else
                    {
                        // Hierarchy view - Roots only
                        if (actor->GetParent() == nullptr)
                        {
                            DrawActorNode(actor.get(), activeSceneMutable, false);
                        }
                    }
                }
            }

            ImGui::TreePop();
        }

        ImGui::Spacing();
        ImGui::Text("%zu actor%s", totalActors, totalActors == 1 ? "" : "s");
        
        if (actorPendingDelete_)
        {
            // Check if we need to clear selection (if selected actor is being deleted or is a child of it)
            if (selectedActorGetter_ && selectedActorSetter_)
            {
                Game::Actor* selected = selectedActorGetter_();
                if (selected && (selected == actorPendingDelete_ || selected->IsChildOf(actorPendingDelete_)))
                {
                    selectedActorSetter_(nullptr);
                }
            }

            activeSceneMutable->RemoveActor(actorPendingDelete_);
            actorPendingDelete_ = nullptr;
        }

        if (openRenamePopup_)
        {
            ImGui::OpenPopup("Rename Actor");
            openRenamePopup_ = false;
        }

        if (ImGui::BeginPopupModal("Rename Actor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter new name:");
            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();
                
            bool enterPressed = ImGui::InputText("##NewName", renameBuffer_.data(), renameBuffer_.size(), ImGuiInputTextFlags_EnterReturnsTrue);

            if (ImGui::Button("OK", ImVec2(120, 0)) || enterPressed)
            {
                if (actorToRename_)
                    actorToRename_->SetName(renameBuffer_.data());
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
                ImGui::CloseCurrentPopup();
            
            ImGui::EndPopup();
        }
    }

    void SceneOutlinerPanel::DrawActorNode(Game::Actor* actor, Game::Scene* scene, bool hasSearch)
    {
        if (!actor) return;

        const String& actorName = actor->GetName();
        const auto actorNameView = actorName.View();
        const String actorType = actor->GetTypeName();
        const auto actorTypeView = actorType.View();
        
        const auto& children = actor->GetChildren();
        const bool hasChildren = !children.empty();

        ImGuiTreeNodeFlags actorFlags =
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                ImGuiTreeNodeFlags_SpanFullWidth;
        
        if (!hasChildren || hasSearch)
        {
            actorFlags |= ImGuiTreeNodeFlags_Leaf;
        }

        Game::Actor* selectedActor = selectedActorGetter_ ? selectedActorGetter_() : nullptr;
        if (selectedActor == actor || actorWithContextMenu_ == actor)
            actorFlags |= ImGuiTreeNodeFlags_Selected;

        bool expanded = false;
        if (!actorNameView.empty())
        {
            expanded = ImGui::TreeNodeEx(actor, actorFlags, "%.*s (%.*s)",
                                         static_cast<int>(actorNameView.size()), actorNameView.data(),
                                         static_cast<int>(actorTypeView.size()), actorTypeView.data());
        }
        else
        {
            expanded = ImGui::TreeNodeEx(actor, actorFlags, "<Unnamed> (%.*s)",
                                         static_cast<int>(actorTypeView.size()), actorTypeView.data());
        }

        // Selection
        if (ImGui::IsItemClicked() && selectedActorSetter_)
            selectedActorSetter_(actor);

        // Drag & Drop Source
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("SCENE_ACTOR", &actor, sizeof(Game::Actor*));
            ImGui::Text("%s", !actorName.IsEmpty() ? actorName.c_str() : "Actor");
            ImGui::EndDragDropSource();
        }

        // Drag & Drop Target
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ACTOR"))
            {
                Game::Actor* droppedActor = *(Game::Actor**)payload->Data;
                if (droppedActor && droppedActor != actor && !droppedActor->IsChildOf(actor))
                {
                    if (CanReparentActor(droppedActor, actor))
                    {
                        OnReparentActor(droppedActor, actor);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Context Menu
        if (ImGui::BeginPopupContextItem())
        {
            actorWithContextMenu_ = actor;
            
            ImGui::TextDisabled("Actor: %.*s", static_cast<int>(actorNameView.size()), actorNameView.data());
            ImGui::Separator();

            if (ImGui::MenuItem("Rename"))
            {
                actorToRename_ = actor;
                const auto& name = actor->GetName();
                const size_t copyLen = std::min(name.length(), renameBuffer_.size() - 1);
                std::memcpy(renameBuffer_.data(), name.View().data(), copyLen);
                renameBuffer_[copyLen] = '\0';
                openRenamePopup_ = true;
            }

            if (CanDeleteActor(actor) && ImGui::MenuItem("Delete"))
            {
                actorPendingDelete_ = actor;
            }
            
            ImGui::EndPopup();
        }
        else if (actorWithContextMenu_ == actor)
        {
            actorWithContextMenu_ = nullptr;
        }

        // Recursion
        if (expanded)
        {
            if (!hasSearch && hasChildren)
            {
                for (auto* child : children)
                {
                    DrawActorNode(child, scene, hasSearch);
                }
            }
            ImGui::TreePop();
        }
    }

    void SceneOutlinerPanel::AddCreatedActor(Game::Scene* scene, std::unique_ptr<Game::Actor> actor)
    {
        if (scene && actor)
        {
            scene->AddActor(std::move(actor));
        }
    }

    void SceneOutlinerPanel::OnReparentActor(Game::Actor* actor, Game::Actor* newParent)
    {
        if (actor)
        {
            actor->SetParent(newParent);
        }
    }
}
