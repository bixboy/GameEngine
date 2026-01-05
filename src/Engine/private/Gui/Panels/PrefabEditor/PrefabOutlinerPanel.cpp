#include "Gui/Panels/PrefabEditor/PrefabOutlinerPanel.h"
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include <utility>

namespace BixEngine::Gui
{
    PrefabOutlinerPanel::PrefabOutlinerPanel(std::function<Game::Scene*()> sceneProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter, std::function<bool(const Game::Actor*)> actorFilter)
        : SceneOutlinerPanel(std::move(sceneProvider), std::move(selectionGetter), std::move(selectionSetter), std::move(actorFilter))
    {
    }

    bool PrefabOutlinerPanel::CanDeleteActor(Game::Actor* actor) const
    {
        
        return actor && actor->GetParent() != nullptr;
    }
    
    bool PrefabOutlinerPanel::CanReparentActor(Game::Actor* actor, Game::Actor* newParent) const
    {
        
        if (actor && actor->GetParent() == nullptr) return false;
        
        
        
        
        
        
        if (newParent == nullptr) return true;
        
        return true;
    }

    void PrefabOutlinerPanel::AddCreatedActor(Game::Scene* scene, std::unique_ptr<Game::Actor> actor)
    {
        if (!scene || !actor) return;
        
        Game::Actor* rawActor = actor.get();
        scene->AddActor(std::move(actor));

        Game::Actor* parent = nullptr;

        
        if (selectedActorGetter_)
        {
            if (auto* selected = selectedActorGetter_())
            {
                
                if (selected != rawActor)
                {
                    parent = selected;
                }
            }
        }

        
        if (!parent)
        {
            const auto& actors = scene->GetActors();
            for (const auto& a : actors)
            {
                
                if (a && a->GetParent() == nullptr && a.get() != rawActor)
                {
                    parent = a.get();
                    break;
                }
            }
        }

        
#include "Gui/Panels/PrefabEditor/PrefabOutlinerPanel.h"
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include <utility>

namespace BixEngine::Gui
{
    PrefabOutlinerPanel::PrefabOutlinerPanel(std::function<Game::Scene*()> sceneProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter, std::function<bool(const Game::Actor*)> actorFilter)
        : SceneOutlinerPanel(std::move(sceneProvider), std::move(selectionGetter), std::move(selectionSetter), std::move(actorFilter))
    {
    }

    bool PrefabOutlinerPanel::CanDeleteActor(Game::Actor* actor) const
    {
        
        return actor && actor->GetParent() != nullptr;
    }
    
    bool PrefabOutlinerPanel::CanReparentActor(Game::Actor* actor, Game::Actor* newParent) const
    {
        
        if (actor && actor->GetParent() == nullptr) return false;
        
        
        
        
        
        
        if (newParent == nullptr) return true;
        
        return true;
    }

    void PrefabOutlinerPanel::AddCreatedActor(Game::Scene* scene, std::unique_ptr<Game::Actor> actor)
    {
        if (!scene || !actor) return;
        
        Game::Actor* rawActor = actor.get();
        scene->AddActor(std::move(actor));

        Game::Actor* parent = nullptr;

        
        if (selectedActorGetter_)
        {
            if (auto* selected = selectedActorGetter_())
            {
                
                if (selected != rawActor)
                {
                    parent = selected;
                }
            }
        }

        
        if (!parent)
        {
            const auto& actors = scene->GetActors();
            for (const auto& a : actors)
            {
                
                if (a && a->GetParent() == nullptr && a.get() != rawActor)
                {
                    parent = a.get();
                    break;
                }
            }
        }

        
        if (parent)
        {
            rawActor->SetParent(parent);
        }
    }

    void PrefabOutlinerPanel::OnReparentActor(Game::Actor* actor, Game::Actor* newParent)
    {
        if (!actor) return;
    
        if (newParent == nullptr)
        {
            
            // In Prefab mode, we can't really reparent to "null" (Scene root) if we are enforcing a single root actor.
            // If rootActor_ is set, we probably shouldn't allow making a sibling of the root.
            if (rootActor_)
            {
                 // Reparent to root instead of scene?
                 // Or just disallow.
                 return;
            }

            Game::Scene* scene = getScene_();
            if(!scene) return;
            
            const auto& actors = scene->GetActors();
            Game::Actor* root = nullptr;
            for (const auto& a : actors)
            {
                 // Trouver le root actuel (Hack pour l'ancien système si pas de rootActor_ set)
                 if (a && a->GetParent() == nullptr) 
                 {
                     root = a.get();
                     break; 
                 }
            }
            
            if (root && root != actor)
            {
                 actor->SetParent(root);
            }
        }
        else
        {
            actor->SetParent(newParent);
        }
    }

    void PrefabOutlinerPanel::Draw()
    {
        if (!rootActor_)
        {
            SceneOutlinerPanel::Draw();
            return;
        }

        ScopedID panelScope("PrefabOutlinerPanel");

        // Simplified Toolbar for Prefab (maybe just Add Child?)
        // Reuse base logic but context is different.
        // For now, let's just draw the tree.
        
        // Search logic replication (simplified)
        SearchInput("SceneOutlinerSearch", searchBuffer_.data(), searchBuffer_.size(), "Search...");
        const String searchQuery(searchBuffer_.data());
        const bool hasSearch = !searchQuery.IsEmpty();

        Game::Scene* activeScene = getScene_ ? getScene_() : nullptr;
        
        // Root Node
        bool match = true; 
        if (hasSearch)
        {
             // Simplistic search: always show root, filtering should be recursive? 
             // SceneOutliner filters linear list.
             // We will just draw the root normally and let the user expand.
             // Or we can implement recursive search later.
        }

        DrawActorNode(rootActor_, activeScene, hasSearch);
        
        // Handle Delete Pending
        if (actorPendingDelete_ && actorPendingDelete_ != rootActor_)
        {
             // Same logic as base
            if (selectedActorGetter_ && selectedActorSetter_)
            {
                Game::Actor* selected = selectedActorGetter_();
                if (selected && (selected == actorPendingDelete_ || selected->IsChildOf(actorPendingDelete_)))
                {
                    selectedActorSetter_(nullptr);
                }
            }
            // Use Scene to remove? In prefab mode, we might just destroy object.
            // activeScene->RemoveActor(actorPendingDelete_); 
            // Warning: if actorPendingDelete_ is child of root, we must ensure usage of Destroy/Remove.
             if (activeScene) activeScene->RemoveActor(actorPendingDelete_);
             actorPendingDelete_ = nullptr;
        }
        
        // Handle Rename
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
}
