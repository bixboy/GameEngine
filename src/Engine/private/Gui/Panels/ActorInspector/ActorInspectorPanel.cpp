#include "Gui/Panels/ActorInspectorPanel.h"
#include "Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"

#include "Actor.h"
#include "Scene.h"


namespace BixEngine::Gui
{
    // ==========================================================================
    ActorInspectorPanel::ActorInspectorPanel(std::function<Game::SceneManager*()> sceneProvider, std::function<Game::Actor*()> selectionGetter,
        std::function<void(Game::Actor*)> selectionSetter)
        : sceneManagerProvider_(std::move(sceneProvider))
        , selectedActorGetter_(std::move(selectionGetter))
        , selectedActorSetter_(std::move(selectionSetter))
        , sections_(ActorInspector::BuildActorInspectorSections())
        , registeredFactoryCount_(ActorInspector::GetRegisteredActorInspectorFactoryCount())
    {
    }

    // Initialisation du panneau
    // ==========================================================================
    void ActorInspectorPanel::OnAttach(GuiPanel& panel)
    {
        GuiPanelController::OnAttach(panel);

        panel.SetResizable(true);
        panel.SetMovable(true);
        panel.SetCollapsable(true);
        panel.SetClosable(true);

        panel.SetBackgroundColor(InspectorBackground);
        panel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);
    }

    // Affichage par frame
    // ==========================================================================
    void ActorInspectorPanel::OnDraw(GuiPanel&)
    {
        ScopedID panelScope("ActorInspectorPanel");

        // Récupération de la scène active
        Game::SceneManager* sceneManager = sceneManagerProvider_ ? sceneManagerProvider_() : nullptr;
        Game::Scene* activeScene = sceneManager ? sceneManager->GetScene() : nullptr;
        if (!activeScene)
        {
            DrawEmptyStateMessage("Aucune scène active.");
            return;
        }

        const std::size_t currentFactoryCount = ActorInspector::GetRegisteredActorInspectorFactoryCount();
        if (currentFactoryCount != registeredFactoryCount_)
        {
            sections_ = ActorInspector::BuildActorInspectorSections();
            registeredFactoryCount_ = currentFactoryCount;
        }

        // Récupération de l’actor sélectionné
        Game::Actor* selectedActor = selectedActorGetter_ ? selectedActorGetter_() : nullptr;
        if (!selectedActor)
        {
            DrawEmptyStateMessage("Aucun actor sélectionné.");
            return;
        }

        // Vérifie si l’actor existe
        if (!ActorInspector::ActorBelongsToScene(*activeScene, selectedActor))
        {
            if (selectedActorSetter_)
                selectedActorSetter_(nullptr);

            DrawEmptyStateMessage("L’actor sélectionné n’existe plus dans cette scène.");
            return;
        }

        // Dessine les sections de l’inspecteur
        for (auto& section : sections_)
        {
            if (section)
                section->Draw(*selectedActor);
        }
    }

    // ==========================================================================
    // Création du panneau dans le GUI Manager
    // ==========================================================================
    GuiPanel& CreateActorInspectorPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        auto registration = guiManager.RegisterUtilityPanel<ActorInspectorPanel>(
            "actor_inspector",
            "Détails",
            context.sceneManagerProvider,
            context.selectedActorGetter,
            context.selectedActorSetter);

        // Docké par défaut sur la droite 
        guiManager.SetPanelDockingArea(registration.panel, DockSpaceRegion::Right);
        return registration.panel;
    }
}
