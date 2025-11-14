#include "Gui/Panels/ActorInspectorPanel.h"

#include "Gui/Panels/ActorInspector/ActorInspectorSection.h"
#include "Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"

#include "Gui/GuiManager.h"
#include "Gui/Internal/GuiPanel.h"
#include "Gui/Controllers/GuiPanelController.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Actor.h"
#include "Scene.h"
#include "SceneManager.h"

#include <imgui.h>
#include <functional>
#include <utility>

#include "Gui/GuiDocking.h"

namespace BixEngine::Gui
{
    using namespace Theme;
    using namespace Utils;

    namespace
    {
        class ActorInspectorPanelController final : public GuiPanelController
        {
        public:
            ActorInspectorPanelController(std::function<Game::SceneManager*()> sceneProvider,
                                          std::function<Game::Actor*()> selectionGetter,
                                          std::function<void(Game::Actor*)> selectionSetter)
                : sceneManagerProvider_(std::move(sceneProvider))
                  , selectedActorGetter_(std::move(selectionGetter))
                  , selectedActorSetter_(std::move(selectionSetter))
                  , sections_(ActorInspector::BuildActorInspectorSections())
                  , registeredFactoryCount_(ActorInspector::GetRegisteredActorInspectorFactoryCount())
            {
            }

            void RegisterSection(ActorInspector::ActorInspectorSectionPtr section)
            {
                if (section)
                {
                    sections_.emplace_back(std::move(section));
                }
            }

        protected:
            void OnAttach(GuiPanel& panel) override
            {
                panel.SetResizable(true);
                panel.SetMovable(true);
                panel.SetCollapsable(true);
                panel.SetClosable(true);
                panel.SetBackgroundColor(InspectorBackground);
                panel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);
            }

            void OnDraw(GuiPanel&) override
            {
                ScopedID panelScope("ActorInspectorPanel");

                Game::SceneManager* sceneManager = sceneManagerProvider_ ? sceneManagerProvider_() : nullptr;
                Game::Scene* activeScene = sceneManager ? sceneManager->GetScene() : nullptr;

                if (!activeScene)
                {
                    DrawEmptyStateMessage("No active scene.");
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
                    DrawEmptyStateMessage("No actor selected.");
                    return;
                }

                if (!ActorInspector::ActorBelongsToScene(*activeScene, selectedActor))
                {
                    if (selectedActorSetter_)
                        selectedActorSetter_(nullptr);

                    DrawEmptyStateMessage("The selected actor is no longer available.");
                    return;
                }

                for (auto& section : sections_)
                {
                    if (section)
                    {
                        section->Draw(*selectedActor);
                    }
                }
            }

        private:
            std::function<Game::SceneManager*()> sceneManagerProvider_{};
            std::function<Game::Actor*()> selectedActorGetter_{};
            std::function<void(Game::Actor*)> selectedActorSetter_{};
            ActorInspector::ActorInspectorSectionList sections_{};
            std::size_t registeredFactoryCount_{0};
        };
    }

    GuiPanel& CreateActorInspectorPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        auto registration = guiManager.RegisterUtilityPanel<ActorInspectorPanelController>(
            "actor_inspector",
            "Details",
            context.sceneManagerProvider,
            context.selectedActorGetter,
            context.selectedActorSetter);

        guiManager.SetPanelDockingArea(registration.panel, DockSpaceRegion::Right);
        return registration.panel;
    }
}
