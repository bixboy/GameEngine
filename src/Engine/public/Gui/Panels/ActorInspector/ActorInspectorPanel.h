#pragma once
#include <functional>
#include "Gui/Core/DefaultEngineGui.h"
#include "Gui/Panels/GuiPanelBase.h"
#include "InspectorSections/ActorInspectorSection.h"


namespace BixEngine::Game
{
    class Actor;
    class SceneManager;
    class Scene;
}

namespace BixEngine::Gui
{
    struct DefaultEngineGuiContext;
    class ActorInspectorPanel : public GuiPanelBase
    {
    public:
        ActorInspectorPanel(std::function<Game::SceneManager*()> sceneManagerProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter);

        ActorInspectorPanel(std::function<Game::Scene*()> sceneProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter);

        explicit ActorInspectorPanel(const DefaultEngineGuiContext& context);

        void DrawBody() override;

    private:
        std::function<Game::SceneManager*()> sceneManagerProvider_; 
        std::function<Game::Scene*()> sceneProvider_;        
        std::function<Game::Actor*()> selectedActorGetter_;
        std::function<void(Game::Actor*)> selectedActorSetter_;

        ActorInspector::ActorInspectorSectionList sections_;
        
        std::size_t registeredFactoryCount_{0};
    };
}