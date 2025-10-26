#include "Engine/Gui/Panels/ActorInspectorPanel.h"

#include "Engine/Gui/Panels/ActorInspector/ActorOverviewUI.h"
#include "Engine/Gui/Panels/ActorInspector/ComponentSectionUI.h"
#include "Engine/Gui/Panels/ActorInspector/ImGuiControls.h"
#include "Engine/Gui/Panels/ActorInspector/TransformSectionUI.h"
#include "Engine/Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"

#include "Engine/Gui/GuiManager.h"
#include "Engine/Gui/GuiPanel.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Game/Actor.h"
#include "Game/Scene.h"
#include "Game/SceneManager.h"

#include <imgui.h>

namespace BixEngine::Gui
{
    GuiPanel& CreateActorInspectorPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        GuiPanel& inspectorPanel = guiManager.CreatePanel("actor_inspector", "Details");
        guiManager.SetPanelDockingArea(inspectorPanel, DockSpaceRegion::Right);
        inspectorPanel.SetResizable(true);
        inspectorPanel.SetMovable(true);
        inspectorPanel.SetCollapsable(true);
        inspectorPanel.SetClosable(true);
        inspectorPanel.SetBackgroundColor(ActorInspector::Colors::kInspectorBackground);
        inspectorPanel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);
        inspectorPanel.SetDrawFunction([provider = context.sceneManagerProvider,
                                        getSelectedActor = context.selectedActorGetter,
                                        setSelectedActor = context.selectedActorSetter]()
        {
            ImGui::PushID("ActorInspectorPanel");

            Game::SceneManager* sceneManager = provider ? provider() : nullptr;
            Game::Scene* activeScene = sceneManager ? sceneManager->GetScene() : nullptr;

            if (!activeScene)
            {
                Utils::DrawEmptyStateMessage("No active scene.");
                ImGui::PopID();
                return;
            }

            Game::Actor* selectedActor = getSelectedActor ? getSelectedActor() : nullptr;
            if (!selectedActor)
            {
                Utils::DrawEmptyStateMessage("No actor selected.");
                ImGui::PopID();
                return;
            }

            if (!ActorInspector::ActorBelongsToScene(*activeScene, selectedActor))
            {
                if (setSelectedActor)
                {
                    setSelectedActor(nullptr);
                }

                Utils::DrawEmptyStateMessage("The selected actor is no longer available.");
                ImGui::PopID();
                return;
            }

            ActorInspector::DrawGeneralSection(*selectedActor);
            ActorInspector::DrawTransformSection(*selectedActor);
            ActorInspector::DrawComponentSection(*selectedActor);

            ImGui::PopID();
        });

        return inspectorPanel;
    }
}

