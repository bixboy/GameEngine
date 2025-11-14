#pragma once
#include "Gui/GuiManager.h"
#include "Gui/Controllers/GuiPanelController.h"
#include "Gui/Utils/GuiHelpers.h"
#include "SceneManager.h"
#include "Actor.h"
#include <functional>
#include "ActorInspector/InspectorSections/ActorInspectorSection.h"
#include "Gui/DefaultEngineGui.h"


namespace BixEngine::Gui
{
    using namespace Theme;
    using namespace Utils;
    
    class ActorInspectorPanel final : public GuiPanelController
    {
    public:
        ActorInspectorPanel(std::function<Game::SceneManager*()> sceneProvider,
            std::function<Game::Actor*()> selectionGetter,
            std::function<void(Game::Actor*)> selectionSetter);

    protected:

        // Called once when panel is created
        // ------------------------------------------------------------------
        void OnAttach(GuiPanel& panel) override;

        // Called every frame
        // ------------------------------------------------------------------
        void OnDraw(GuiPanel&) override;

    private:
        // ----------------------------------------------------------------------
        // Sources de données externes (fournies par l’éditeur)
        // ----------------------------------------------------------------------
        std::function<Game::SceneManager*()> sceneManagerProvider_;
        std::function<Game::Actor*()>        selectedActorGetter_;
        std::function<void(Game::Actor*)>    selectedActorSetter_;

        // ----------------------------------------------------------------------
        // Sections affichées dans l’inspecteur (Général, Transform, Components, plugins...)
        // ----------------------------------------------------------------------
        ActorInspector::ActorInspectorSectionList sections_;
        std::size_t registeredFactoryCount_{0};
    };

    /**
     * @brief Fonction utilitaire utilisée par l’éditeur pour créer le panneau.
     */
    GuiPanel& CreateActorInspectorPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}
