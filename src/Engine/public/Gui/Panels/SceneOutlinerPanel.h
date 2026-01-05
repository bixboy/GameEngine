#pragma once
#include <array>
#include <functional>

#include "GuiPanelBase.h"
#include "Gui/Core/DefaultEngineGui.h"


namespace BixEngine::Game
{
    class Actor;
    class Scene;
    class SceneManager;
}


namespace BixEngine::Gui
{
    class SceneOutlinerPanel : public GuiPanelBase
    {
    public:
        SceneOutlinerPanel(std::function<Game::Scene*()> sceneProvider, std::function<Game::Actor*()> selectionGetter,
            std::function<void(Game::Actor*)> selectionSetter, std::function<bool(const Game::Actor*)> actorFilter = nullptr);
        
        explicit SceneOutlinerPanel(const DefaultEngineGuiContext& context);
        virtual ~SceneOutlinerPanel() = default;

        void DrawBody() override;

    protected:
        std::function<Game::Scene*()> getScene_{};
        std::function<Game::Actor*()> selectedActorGetter_{};
        std::function<void(Game::Actor*)> selectedActorSetter_{};
        std::function<bool(const Game::Actor*)> actorFilter_{nullptr};
        
        std::array<char, 128> searchBuffer_{};
        std::array<char, 128> renameBuffer_{};
        
        Game::Actor* actorToRename_{nullptr};
        bool openRenamePopup_{false};

        Game::Actor* actorWithContextMenu_{nullptr};
        Game::Actor* actorPendingDelete_{nullptr};

        virtual void DrawActorNode(Game::Actor* actor, Game::Scene* scene, bool hasSearch);
        
        virtual bool CanDeleteActor(Game::Actor* actor) const { return true; }
        virtual bool CanReparentActor(Game::Actor* movedActor, Game::Actor* newParent) const { return true; }
        virtual void AddCreatedActor(Game::Scene* scene, std::unique_ptr<Game::Actor> actor);
        virtual void OnReparentActor(Game::Actor* movedActor, Game::Actor* newParent);
    };
}