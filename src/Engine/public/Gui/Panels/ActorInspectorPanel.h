#pragma once
#include <functional>

#include "Gui/Base/GuiPanelBase.h"
#include "Gui/Utils/GuiHelpers.h"
#include "ActorInspector/InspectorSections/ActorInspectorSection.h"
#include "Gui/Core/DefaultEngineGui.h"

namespace BixEngine::Game
{
    class Actor;
    class SceneManager;
    class Scene;
}


namespace BixEngine::Gui
{
    class ActorInspectorPanel : public GuiPanelBase
    {
    public:
        ActorInspectorPanel(std::function<Game::SceneManager*()> sceneManagerProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter);
        ActorInspectorPanel(std::function<Game::Scene*()> sceneProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter);

        explicit ActorInspectorPanel(const DefaultEngineGuiContext& context);

        void Draw() override;

    private:
        // ----------------------------------------------------------------------
        // Sources de données externes (fournies par l’éditeur)
        // ----------------------------------------------------------------------
        std::function<Game::SceneManager*()> sceneManagerProvider_; // Optional fallback
        std::function<Game::Scene*()>        sceneProvider_;        // Primary source
        std::function<Game::Actor*()>        selectedActorGetter_;
        std::function<void(Game::Actor*)>    selectedActorSetter_;

        // ----------------------------------------------------------------------
        // Sections affichées dans l’inspecteur (Général, Transform, Components, plugins...)
        // ----------------------------------------------------------------------
        ActorInspector::ActorInspectorSectionList sections_;
        std::size_t registeredFactoryCount_{0};
    };
}
