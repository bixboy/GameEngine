#pragma once

#include "Gui/Panels/SceneOutlinerPanel.h"

namespace BixEngine::Gui
{
    class PrefabOutlinerPanel : public SceneOutlinerPanel
    {
    public:
        PrefabOutlinerPanel(std::function<Game::Scene*()> sceneProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter, std::function<bool(const Game::Actor*)> actorFilter = nullptr);
        virtual ~PrefabOutlinerPanel() = default;

        
        bool CanDeleteActor(Game::Actor* actor) const override;
        bool CanReparentActor(Game::Actor* actor, Game::Actor* newParent) const override;
        void AddCreatedActor(Game::Scene* scene, std::unique_ptr<Game::Actor> actor) override;
        void OnReparentActor(Game::Actor* actor, Game::Actor* newParent) override;
    };
}
