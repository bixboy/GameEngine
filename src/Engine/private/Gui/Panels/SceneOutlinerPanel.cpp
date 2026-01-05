#include "Gui/Panels/SceneOutlinerPanel.h"
#include <utility>
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include "Framework/SceneManager.h"
#include "Gui/Utils/GuiHelpers.h"
#include "imgui.h"
#include "Serializer/SceneSerializer.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Utils/Editor/ScriptIntrospector.h"
#include "Utils/Editor/ScriptUtils.h"
#include "Debug/Logger.h"
#include "Utils/FileIO/BinaryUtils.h"
#include <fstream>
#include <filesystem>
#include "Gui/Widgets/Layout/PanelToolbar.h"


namespace BixEngine::Gui
{
    using namespace Utils;

    SceneOutlinerPanel::SceneOutlinerPanel(std::function<Game::Scene*()> sceneProvider, std::function<Game::Actor*()> selectionGetter,
        std::function<void(Game::Actor*)> selectionSetter, std::function<bool(const Game::Actor*)> actorFilter)
        : GuiPanelBase("scene_outliner"), 
          getScene_(std::move(sceneProvider)), 
          selectedActorGetter_(std::move(selectionGetter)),
          selectedActorSetter_(std::move(selectionSetter)), 
          actorFilter_(std::move(actorFilter))
    {
        searchBuffer_.fill('\0');
        renameBuffer_.fill('\0');
    }

    SceneOutlinerPanel::SceneOutlinerPanel(const DefaultEngineGuiContext& context) : SceneOutlinerPanel(
        [context]() -> Game::Scene*
        {
            return context.sceneManagerProvider ? context.sceneManagerProvider()->GetScene() : nullptr;
        }, 
        context.selectedActorGetter, 
        context.selectedActorSetter,
        nullptr)
    {
    }

    void SceneOutlinerPanel::DrawBody()
    {
        GuiUtils::ScopedID panelScope("SceneOutlinerPanel");

        Game::Scene* activeScene = getScene_ ? getScene_() : nullptr;
        if (!activeScene)
        {
            GuiUtils::DrawEmptyStateMessage("No active scene.");
            return;
        }

        Widgets::Layout::PanelToolbar toolbar;
        toolbar.AddLeft([this, activeScene]()
        {
            if (ImGui::Button("+"))
                ImGui::OpenPopup("AddActorPopup");

            if (ImGui::BeginPopup("AddActorPopup"))
            {
                if (ImGui::MenuItem("Empty Actor"))
                    activeScene->SpawnActor<Game::Actor>("New Actor");

                ImGui::Separator();
                
                static std::vector<Editor::ScriptIntrospector::PrefabScriptCandidate> candidates;
                if (ImGui::IsWindowAppearing())
                {
                    candidates.clear();
                    auto bases = Editor::ScriptIntrospector::GetBaseClasses();
                    
                    auto* contentBrowser = ContentBrowserPanel::GetActiveInstance();
                    if (contentBrowser)
                    {
                         std::filesystem::path contentRoot = std::filesystem::current_path() / "Content";
                         std::filesystem::path scriptsDir = contentRoot / "Scripts";
                         std::vector<ScriptUtils::ScriptNode> scriptRoots;
                        
                         if (std::filesystem::exists(scriptsDir))
                             scriptRoots = ScriptUtils::Utilities::BuildScriptTree(scriptsDir, contentRoot);
                        
                         candidates = Editor::ScriptIntrospector::GatherPrefabCandidates(scriptRoots, bases);
                         
                         if (std::filesystem::exists(contentRoot))
                         {
                            for (const auto& entry : std::filesystem::recursive_directory_iterator(contentRoot))
                            {
                                if (entry.is_regular_file() && entry.path().extension() == ".bixactor")
                                {
                                    Editor::ScriptIntrospector::PrefabScriptCandidate prefabCandidate;
                                    prefabCandidate.displayName = entry.path().stem().string();
                                    prefabCandidate.className = entry.path().string(); 
                                    prefabCandidate.isActor = true;
                                    prefabCandidate.isComponent = false;
                                    prefabCandidate.assetBaseName = "Prefab"; 
                                    candidates.push_back(prefabCandidate);
                                }
                            }
                        }
                    }
                    else
                    {
                        candidates = Editor::ScriptIntrospector::GatherPrefabCandidates({}, bases);
                    }
                }

                if (ImGui::BeginMenu("Add Actor from Script"))
                {
                    for (int i = 0; i < candidates.size(); ++i)
                    {
                        const auto& candidate = candidates[i];
                        if (!candidate.isActor)
                            continue;
                        
                        ImGui::PushID(i);
                        bool isPrefab = candidate.assetBaseName == "Prefab";
                        if (ImGui::MenuItem(candidate.displayName.c_str(), isPrefab ? "Prefab" : nullptr))
                        {
                            if (isPrefab)
                            {
                                std::filesystem::path path(candidate.className);
                                if (path.extension() == ".bixactor")
                                {
                                     std::ifstream file(path, std::ios::binary);
                                     if (file.is_open())
                                     {
                                         BinaryReader reader(file);
                                         String typeName;
                                         if (reader.ReadString(typeName) && !typeName.empty())
                                         {
                                             auto newActor = Serialization::SceneSerializer::CreateActor(typeName);
                                             if (newActor)
                                             {
                                                 newActor->SetName(candidate.displayName.c_str());
                                                 try
                                                 {
                                                     newActor->DeserializeBinary(file);
                                                     activeScene->AddActor(std::move(newActor));
                                                 }
                                                 catch(...)
                                                 {
                                                     LOG_ERROR("Failed to deserialize actor: " + candidate.displayName);
                                                 }
                                             }
                                         }
                                     }
                                }
                                else
                                {
                                    // Fallback parsing texte (Legacy ou format spécifique)
                                    // ... (Ta logique existante de parsing manuel) ...
                                }
                            } 
                            else 
                            {
                                auto newActor = Serialization::SceneSerializer::CreateActor(candidate.className.c_str());
                                if (newActor)
                                { 
                                    newActor->SetName(candidate.displayName.c_str()); 
                                    AddCreatedActor(activeScene, std::move(newActor));
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
            GuiUtils::SearchInput("SceneOutlinerSearch", searchBuffer_.data(), searchBuffer_.size(), "Search actors...");
        });
        toolbar.Commit();

        const String searchQuery(searchBuffer_.data());
        const bool hasSearch = !searchQuery.empty();

        const auto& actors = activeScene->GetActors();
        std::size_t totalActors = 0;
        
        for (const auto& actor : actors) 
        {
            if (actor && (!actorFilter_ || actorFilter_(actor.get())))
                totalActors++;
        }

        const String& sceneName = activeScene->GetName();
        const auto sceneNameView = sceneName.View();
        
        constexpr ImGuiTreeNodeFlags sceneFlags =
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanFullWidth;

        if (ImGui::TreeNodeEx(activeScene, sceneFlags, "%.*s", static_cast<int>(sceneNameView.size()), sceneNameView.data()))
        {
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ACTOR"))
                {
                    Game::Actor* droppedActor = *static_cast<Game::Actor**>(payload->Data);
                    if (droppedActor && CanReparentActor(droppedActor, nullptr))
                    {
                        OnReparentActor(droppedActor, nullptr);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (ImGui::IsItemClicked() && selectedActorSetter_)
                selectedActorSetter_(nullptr);

            if (totalActors == 0)
            {
                GuiUtils::DrawEmptyStateMessage("No actors in this scene.");
            }
            else
            {
                for (const auto& actor : actors)
                {
                    if (!actor) continue;
                    if (actorFilter_ && !actorFilter_(actor.get())) continue;

                    if (hasSearch)
                    {
                         const String& actorName = actor->GetName();
                         String typeName(actor->GetTypeName());
                         bool match = (!actorName.empty() && actorName.Contains(searchQuery.View(), false)) || typeName.Contains(searchQuery.View(), false);
                         
                         if (match)
                            DrawActorNode(actor.get(), activeScene, true);
                    }
                    else
                    {
                        if (actor->GetParent() == nullptr)
                        {
                            DrawActorNode(actor.get(), activeScene, false);
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
            if (selectedActorGetter_ && selectedActorSetter_)
            {
                Game::Actor* selected = selectedActorGetter_();
                if (selected && (selected == actorPendingDelete_ || selected->IsChildOf(actorPendingDelete_)))
                {
                    selectedActorSetter_(nullptr);
                }
            }
            
            activeScene->RemoveActor(actorPendingDelete_);
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
        const String actorType = actor->GetTypeName();
        const auto& children = actor->GetChildren();
        const bool hasChildren = !children.empty();

        ImGuiTreeNodeFlags actorFlags =ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth;
        
        if (!hasChildren || hasSearch)
        {
            actorFlags |= ImGuiTreeNodeFlags_Leaf;
        }

        Game::Actor* selectedActor = selectedActorGetter_ ? selectedActorGetter_() : nullptr;
        if (selectedActor == actor || actorWithContextMenu_ == actor)
            actorFlags |= ImGuiTreeNodeFlags_Selected;

        bool expanded = false;
        if (!actorName.empty())
        {
            expanded = ImGui::TreeNodeEx(actor, actorFlags, "%.*s (%.*s)", 
                static_cast<int>(actorName.length()), actorName.c_str(), 
                static_cast<int>(actorType.length()), actorType.c_str());
        }
        else
        {
            expanded = ImGui::TreeNodeEx(actor, actorFlags, "<Unnamed> (%.*s)", 
                static_cast<int>(actorType.length()), actorType.c_str());
        }

        if (ImGui::IsItemClicked() && selectedActorSetter_)
            selectedActorSetter_(actor);

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("SCENE_ACTOR", &actor, sizeof(Game::Actor*));
            ImGui::Text("%s", !actorName.empty() ? actorName.c_str() : "Actor");
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ACTOR"))
            {
                Game::Actor* droppedActor = *static_cast<Game::Actor**>(payload->Data);
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
            ImGui::TextDisabled("Actor: %s", actorName.c_str());
            ImGui::Separator();

            if (ImGui::MenuItem("Rename"))
            {
                actorToRename_ = actor;
                const auto& name = actor->GetName();
                
                const size_t nameLen = std::strlen(name.c_str());
                const size_t copyLen = std::min(nameLen, renameBuffer_.size() - 1);
                
                std::memcpy(renameBuffer_.data(), name.c_str(), copyLen);
                
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
            scene->AddActor(std::move(actor));
    }

    void SceneOutlinerPanel::OnReparentActor(Game::Actor* movedActor, Game::Actor* newParent)
    {
        if (movedActor)
            movedActor->SetParent(newParent);
    }
}