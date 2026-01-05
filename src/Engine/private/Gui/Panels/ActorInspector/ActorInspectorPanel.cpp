#include "Gui/Panels/ActorInspector/ActorInspectorPanel.h"
#include <utility>
#include "Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"
#include "Framework/Actor.h"
#include "Framework/SceneManager.h"
#include "Framework/Scene.h"
#include "Gui/Panels/ActorInspector/InspectorSections/ActorInspectorSection.h"
#include "Gui/Utils/GuiHelpers.h"


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

    ActorInspectorPanel::ActorInspectorPanel(const DefaultEngineGuiContext& context): GuiPanelBase("actor_inspector"),
          sceneManagerProvider_(context.sceneManagerProvider),
          selectedActorGetter_(context.selectedActorGetter),
          selectedActorSetter_(context.selectedActorSetter),
          sections_(ActorInspector::BuildActorInspectorSections()),
          registeredFactoryCount_(ActorInspector::GetRegisteredActorInspectorFactoryCount())
    {
    }

    void ActorInspectorPanel::DrawBody()
    {
        GuiUtils::ScopedID panelScope("ActorInspectorPanel");

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
            GuiUtils::DrawEmptyStateMessage("Aucune scène active.");
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
            GuiUtils::DrawEmptyStateMessage("Aucun acteur sélectionné.");
            return;
        }

        if (!ActorInspector::ActorBelongsToScene(*activeScene, selectedActor))
        {
            if (selectedActorSetter_)
                selectedActorSetter_(nullptr);

            GuiUtils::DrawEmptyStateMessage("La sélection est invalide ou n'appartient pas à la scène.");
            return;
        }

        for (auto& section : sections_)
        {
            if (section)
            {
                // ImGui::PushID(section->GetSectionID()); 
                section->Draw(*selectedActor);
                // ImGui::PopID();
            }
        }
    }
}
