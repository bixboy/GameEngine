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
        // Root cannot be deleted
        return actor && actor->GetParent() != nullptr;
    }
    
    bool PrefabOutlinerPanel::CanReparentActor(Game::Actor* actor, Game::Actor* newParent) const
    {
        // Root cannot be reparented (must stay root)
        if (actor && actor->GetParent() == nullptr) return false;
        
        // Items cannot be unparented (must stay inside prefab hierarchy, so newParent must not be null)
        // User said: "actor ... soit le parent de tout ce que l'ont ajoute" -> All must be children of Root.
        // So newParent must be implicitly part of the prefab. 
        // If newParent is nullptr, it means making it a root sibling. Banned normally, BUT we treat it as "Reparent to Root".
        // So we ALLOW nullptr now, and OnReparentActor will handle redirection.
        if (newParent == nullptr) return true;
        
        return true;
    }

    void PrefabOutlinerPanel::AddCreatedActor(Game::Scene* scene, std::unique_ptr<Game::Actor> actor)
    {
        if (!scene || !actor) return;
        
        Game::Actor* rawActor = actor.get();
        scene->AddActor(std::move(actor));

        Game::Actor* parent = nullptr;

        // 1. Try to parent to the currently selected actor
        if (selectedActorGetter_)
        {
            if (auto* selected = selectedActorGetter_())
            {
                // Ensure we don't parent to itself
                if (selected != rawActor)
                {
                    parent = selected;
                }
            }
        }

        // 2. If no parent selected (or selection was invalid), fallback to the Prefab Root
        if (!parent)
        {
            const auto& actors = scene->GetActors();
            for (const auto& a : actors)
            {
                // Find the existing root (actor with no parent, excluding the new one)
                if (a && a->GetParent() == nullptr && a.get() != rawActor)
                {
                    parent = a.get();
                    break;
                }
            }
        }

        // 3. Apply parent
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
            // Redirect to Root
            // Find Root.
            Game::Scene* scene = getScene_();
            if(!scene) return;
            
            const auto& actors = scene->GetActors();
            Game::Actor* root = nullptr;
            for (const auto& a : actors)
            {
                 // Root is the one without parent
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
}
