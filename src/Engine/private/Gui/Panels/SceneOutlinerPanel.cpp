#include "Gui/Panels/SceneOutlinerPanel.h"
#include <utility>
#include "Actor.h"
#include "Scene.h"
#include "SceneManager.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Widgets/Widgets.h"
#include "imgui.h"
#include "SceneSerializer.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Utils/PrefabUtils.h"
#include "Utils/ScriptUtils.h"
#include "Logger.h"
#include <fstream>
#include <filesystem>


namespace BixEngine::Gui
{
    using namespace Utils;

    SceneOutlinerPanel::SceneOutlinerPanel(std::function<Game::SceneManager*()> sceneProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter)
        : GuiPanelBase("scene_outliner"),
        sceneManagerProvider_(std::move(sceneProvider)),
        selectedActorGetter_(std::move(selectionGetter)),
        selectedActorSetter_(std::move(selectionSetter))
    {
        searchBuffer_.fill('\0');
    }

    SceneOutlinerPanel::SceneOutlinerPanel(const DefaultEngineGuiContext& context) : SceneOutlinerPanel(context.sceneManagerProvider, context.selectedActorGetter, context.selectedActorSetter)
    {
    }

    void SceneOutlinerPanel::Draw()
    {
        ScopedID panelScope("SceneOutlinerPanel");

        const Game::SceneManager* sceneManager = sceneManagerProvider_ ? sceneManagerProvider_() : nullptr;
        const Game::Scene* activeScene = sceneManager ? sceneManager->GetScene() : nullptr;
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
                {
                    activeSceneMutable->SpawnActor<Game::Actor>("New Actor");
                }

                ImGui::Separator();
                ImGui::TextDisabled("From Script");

                static std::vector<PrefabUtils::Utilities::PrefabScriptCandidate> candidates;
                
                // Refresh candidates when opening
                if (ImGui::IsWindowAppearing())
                {
                    candidates.clear();
                    
                    // 1. Base Classes
                    auto bases = PrefabUtils::Utilities::GetBaseClasses();
                    
                    // 2. User Scripts & Prefabs
                    std::vector<ScriptUtils::ScriptNode> scriptRoots;
                    
                    // Try to get content root from Content Browser
                    auto* contentBrowser = ContentBrowserPanel::GetActiveInstance();
                    if (contentBrowser)
                    {
                        // Assume "Content" folder relative to CWD
                        std::filesystem::path contentRoot = std::filesystem::current_path() / "Content";
                        std::filesystem::path scriptsDir = contentRoot / "Scripts";
                        
                        if (std::filesystem::exists(scriptsDir))
                        {
                            scriptRoots = ScriptUtils::Utilities::BuildScriptTree(scriptsDir, contentRoot);
                        }
                        
                        candidates = PrefabUtils::Utilities::GatherPrefabCandidates(scriptRoots, bases);

                        // Manual Prefab Scan (.bixactor files)
                        if (std::filesystem::exists(contentRoot))
                        {
                            for (const auto& entry : std::filesystem::recursive_directory_iterator(contentRoot))
                            {
                                if (entry.is_regular_file() && entry.path().extension() == ".bixactor")
                                {
                                    PrefabUtils::Utilities::PrefabScriptCandidate prefabCandidate;
                                    prefabCandidate.displayName = entry.path().stem().string();
                                    prefabCandidate.className = entry.path().string(); // Store path in className
                                    prefabCandidate.isActor = true;
                                    prefabCandidate.isComponent = false;
                                    prefabCandidate.assetBaseName = "Prefab"; // Marker
                                    candidates.push_back(prefabCandidate);
                                }
                            }
                        }
                    }
                    else
                    {
                        candidates = PrefabUtils::Utilities::GatherPrefabCandidates({}, bases);
                    }
                }

                if (ImGui::BeginMenu("Add Actor from Script"))
                {
                    for (const auto& candidate : candidates)
                    {
                        if (!candidate.isActor) continue;
                        
                        bool isPrefab = candidate.assetBaseName == "Prefab";

                        if (ImGui::MenuItem(candidate.displayName.c_str(), isPrefab ? "Prefab" : nullptr))
                        {
                            if (isPrefab)
                            {
                                // Spawn Prefab - parse JSON to find class name
                                std::ifstream file(candidate.className);
                                if (file.is_open())
                                {
                                    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                                    
                                    // Simple JSON parsing to find "class": "ClassName"
                                    std::string searchKey = "\"class\"";
                                    size_t pos = content.find(searchKey);
                                    if (pos != std::string::npos)
                                    {
                                        size_t startQuote = content.find('"', pos + searchKey.length());
                                        // Skip the colon and whitespace
                                        while (startQuote != std::string::npos && (content[startQuote] == ':' || isspace(content[startQuote])))
                                        {
                                            startQuote = content.find('"', startQuote + 1);
                                        }

                                        if (startQuote != std::string::npos)
                                        {
                                            size_t endQuote = content.find('"', startQuote + 1);
                                            if (endQuote != std::string::npos)
                                            {
                                                std::string className = content.substr(startQuote + 1, endQuote - startQuote - 1);
                                                
                                                auto newActor = Game::SceneSerializer::CreateActor(className.c_str());
                                                if (newActor)
                                                {
                                                    newActor->SetName(candidate.displayName.c_str());
                                                    activeSceneMutable->AddActor(std::move(newActor));
                                                }
                                                else
                                                {
                                                    LOG_ERROR("Failed to spawn prefab actor class: " + String(className.c_str()));
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        LOG_ERROR("Invalid prefab file format (missing 'class'): " + String(candidate.className.c_str()));
                                    }
                                }
                                else
                                {
                                    LOG_ERROR("Could not open prefab file: " + String(candidate.className.c_str()));
                                }
                            }
                            else
                            {
                                // Spawn C++ Class
                                auto newActor = Game::SceneSerializer::CreateActor(candidate.className.c_str());
                                if (newActor)
                                {
                                    newActor->SetName(candidate.displayName.c_str());
                                    activeSceneMutable->AddActor(std::move(newActor));
                                }
                                else
                                {
                                    LOG_WARNING("Could not spawn actor of type: " + String(candidate.className.c_str()) + ". Ensure it is registered in SceneSerializer.");
                                }
                            }
                        }
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
        Game::Actor* selectedActor = selectedActorGetter_ ? selectedActorGetter_() : nullptr;

        const auto matchesFilter = [&searchQuery, hasSearch](const Game::Actor& actor)
        {
            if (!hasSearch)
                return true;

            const String& actorName = actor.GetName();
            if (!actorName.IsEmpty() && actorName.Contains(searchQuery.View(), false))
                return true;

            String typeName(actor.GetTypeName());
            return typeName.Contains(searchQuery.View(), false);
        };

        std::size_t totalActors = 0;
        std::size_t filteredActors = 0;
        Game::Actor* actorToDelete = nullptr;

        for (const auto& actor : actors)
        {
            if (!actor)
                continue;

            ++totalActors;

            if (matchesFilter(*actor))
                ++filteredActors;
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
                    if (!actor || !matchesFilter(*actor))
                        continue;

                    const String& actorName = actor->GetName();
                    const auto actorNameView = actorName.View();
                    const String actorType = actor->GetTypeName();
                    const auto actorTypeView = actorType.View();

                    const bool hasName = !actorNameView.empty();
                    ImGuiTreeNodeFlags actorFlags =
                        ImGuiTreeNodeFlags_Leaf |
                        ImGuiTreeNodeFlags_NoTreePushOnOpen |
                        ImGuiTreeNodeFlags_SpanFullWidth;

                    if (selectedActor == actor.get() || actorWithContextMenu_ == actor.get())
                        actorFlags |= ImGuiTreeNodeFlags_Selected;

                    if (hasName)
                    {
                        ImGui::TreeNodeEx(actor.get(), actorFlags, "%.*s (%.*s)",
                            static_cast<int>(actorNameView.size()), actorNameView.data(),
                            static_cast<int>(actorTypeView.size()), actorTypeView.data());
                    }
                    else
                    {
                        ImGui::TreeNodeEx(actor.get(), actorFlags, "<Unnamed> (%.*s)",
                            static_cast<int>(actorTypeView.size()), actorTypeView.data());
                    }

                    if (ImGui::IsItemClicked() && selectedActorSetter_)
                        selectedActorSetter_(actor.get());

                    bool isContextMenuOpen = false;
                    if (ImGui::BeginPopupContextItem())
                    {
                        isContextMenuOpen = true;
                        actorWithContextMenu_ = actor.get();
                        
                        ImGui::TextDisabled("Actor: %.*s", static_cast<int>(actorNameView.size()), actorNameView.data());
                        ImGui::Separator();

                        if (ImGui::MenuItem("Rename"))
                        {
                            actorToRename_ = actor.get();
                            const auto& name = actor->GetName();
                            // Safe copy
                            const size_t copyLen = std::min(name.length(), renameBuffer_.size() - 1);
                            std::memcpy(renameBuffer_.data(), name.View().data(), copyLen);
                            renameBuffer_[copyLen] = '\0';
                            openRenamePopup_ = true;
                        }

                        if (ImGui::MenuItem("Delete"))
                        {
                            actorToDelete = actor.get();
                        }
                        
                        ImGui::EndPopup();
                    }
                    else if (actorWithContextMenu_ == actor.get())
                    {
                        // Popup closed, clear highlight
                        actorWithContextMenu_ = nullptr;
                    }
                }

                if (hasSearch && filteredActors == 0)
                    DrawEmptyStateMessage("No actors match the current filter.");
            }

            ImGui::TreePop();
        }

        ImGui::Spacing();
        if (hasSearch)
            ImGui::Text("%zu / %zu actor%s", filteredActors, totalActors, totalActors == 1 ? "" : "s");
        else
            ImGui::Text("%zu actor%s", totalActors, totalActors == 1 ? "" : "s");
        
        if (actorToDelete)
        {
            activeSceneMutable->RemoveActor(actorToDelete);
            // If the deleted actor was selected, clear selection
            if (selectedActor == actorToDelete && selectedActorSetter_)
                selectedActorSetter_(nullptr);
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
                {
                    actorToRename_->SetName(renameBuffer_.data());
                }
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }
}
