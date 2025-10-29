#include "Engine/Gui/Panels/ActorInspectorPanel.h"

#include "Engine/Gui/Panels/ActorInspector/ActorOverviewUI.h"
#include "Engine/Gui/Panels/ActorInspector/ComponentSectionUI.h"
#include "Engine/Gui/Panels/ActorInspector/ImGuiControls.h"
#include "Engine/Gui/Panels/ActorInspector/TransformSectionUI.h"
#include "Engine/Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"

#include "Engine/Gui/Core/GuiManager.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Gui/Controllers/GuiPanelController.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Game/Actor.h"
#include "Game/Scene.h"
#include "Game/SceneManager.h"

#include <imgui.h>
#include <functional>

#include "Engine/Gui/Core/GuiDocking.h"

namespace BixEngine::Gui
{
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
            {
            }

        protected:
            void OnAttach(GuiPanel& panel) override
            {
                panel.SetResizable(true);
                panel.SetMovable(true);
                panel.SetCollapsable(true);
                panel.SetClosable(true);
                panel.SetBackgroundColor(ActorInspector::Colors::kInspectorBackground);
                panel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);
            }

            void OnDraw(GuiPanel&) override
            {
                ImGui::PushID("ActorInspectorPanel");

                Game::SceneManager* sceneManager = sceneManagerProvider_ ? sceneManagerProvider_() : nullptr;
                Game::Scene* activeScene = sceneManager ? sceneManager->GetScene() : nullptr;

                if (!activeScene)
                {
                    Utils::DrawEmptyStateMessage("No active scene.");
                    ImGui::PopID();
                    return;
                }

                Game::Actor* selectedActor = selectedActorGetter_ ? selectedActorGetter_() : nullptr;
                if (!selectedActor)
                {
                    Utils::DrawEmptyStateMessage("No actor selected.");
                    ImGui::PopID();
                    return;
                }

                if (!ActorInspector::ActorBelongsToScene(*activeScene, selectedActor))
                {
                    if (selectedActorSetter_)
                        selectedActorSetter_(nullptr);

                    Utils::DrawEmptyStateMessage("The selected actor is no longer available.");
                    ImGui::PopID();
                    return;
                }

                ActorInspector::DrawGeneralSection(*selectedActor);
                ActorInspector::DrawTransformSection(*selectedActor);
                ActorInspector::DrawComponentSection(*selectedActor);

                ImGui::PopID();
            }

        private:
            std::function<Game::SceneManager*()> sceneManagerProvider_{};
            std::function<Game::Actor*()> selectedActorGetter_{};
            std::function<void(Game::Actor*)> selectedActorSetter_{};
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

