#include "Gui/Panels/ActorInspectorPanel.h"

#include <utility>

#include "Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"

#include "Framework/Actor.h"
#include "Framework/Scene.h"

namespace BixEngine::Gui
{
    using namespace Utils;

    ActorInspectorPanel::ActorInspectorPanel(std::function<Game::SceneManager*()> sceneManagerProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter)
        : GuiPanelBase("actor_inspector"),
          sceneManagerProvider_(std::move(sceneManagerProvider)),
          selectedActorGetter_(std::move(selectionGetter)),
          selectedActorSetter_(std::move(selectionSetter)),
          sections_(ActorInspector::BuildActorInspectorSections()),
          registeredFactoryCount_(ActorInspector::GetRegisteredActorInspectorFactoryCount())
    {
    }

    ActorInspectorPanel::ActorInspectorPanel(std::function<Game::Scene*()> sceneProvider, std::function<Game::Actor*()> selectionGetter, std::function<void(Game::Actor*)> selectionSetter)
        : GuiPanelBase("actor_inspector"),
          sceneProvider_(std::move(sceneProvider)),
          selectedActorGetter_(std::move(selectionGetter)),
          selectedActorSetter_(std::move(selectionSetter)),
          sections_(ActorInspector::BuildActorInspectorSections()),
          registeredFactoryCount_(ActorInspector::GetRegisteredActorInspectorFactoryCount())
    {
    }

    ActorInspectorPanel::ActorInspectorPanel(const DefaultEngineGuiContext& context)
        : GuiPanelBase("actor_inspector"),
          sceneManagerProvider_(context.sceneManagerProvider), 
          sceneProvider_(context.sceneProvider),               
          selectedActorGetter_(context.selectedActorGetter),
          selectedActorSetter_(context.selectedActorSetter),
          sections_(ActorInspector::BuildActorInspectorSections()),
          registeredFactoryCount_(ActorInspector::GetRegisteredActorInspectorFactoryCount())
    {
    }

    void ActorInspectorPanel::Draw()
    {
        ScopedID panelScope("ActorInspectorPanel");

        Game::Scene* activeScene = nullptr;
        
        if (sceneProvider_)
        {
            activeScene = sceneProvider_();
        }
        else if (sceneManagerProvider_)
        {
             if (auto* sm = sceneManagerProvider_())
                 activeScene = sm->GetScene();
        }

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

        Game::Actor* selectedActor = selectedActorGetter_ ? selectedActorGetter_() : nullptr;
        if (!selectedActor)
        {
            DrawEmptyStateMessage("Aucun actor sélectionné.");
            return;
        }

        if (!ActorInspector::ActorBelongsToScene(*activeScene, selectedActor))
        {
            if (selectedActorSetter_)
                selectedActorSetter_(nullptr);

            DrawEmptyStateMessage("L’actor sélectionné n’existe plus dans cette scène.");
            return;
        }

        for (auto& section : sections_)
        {
            if (section)
                section->Draw(*selectedActor);
        }
    }
}
