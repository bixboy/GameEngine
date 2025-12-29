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
            
            
            Game::Scene* scene = getScene_();
            if(!scene) return;
            
            const auto& actors = scene->GetActors();
            Game::Actor* root = nullptr;
            for (const auto& a : actors)
            {
                 
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
