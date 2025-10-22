#include "Bix/Engine/Gui/Panels/ActorInspectorPanel.h"

#include "Bix/Game/Actor.h"
#include "Bix/Game/Components/Component.h"
#include "Bix/Game/Scene.h"
#include "Bix/Game/SceneManager.h"
#include "Bix/Engine/Gui/GuiManager.h"
#include "Bix/Engine/Gui/GuiPanel.h"
#include "Bix/Engine/Gui/Utils/GuiHelpers.h"

#include "imgui.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>

namespace BixEngine::Gui
{
    namespace
    {
        constexpr ImVec4 kInspectorBackground{0.12f, 0.12f, 0.12f, 0.95f};

        namespace Utils = BixEngine::Gui::Utils;

        bool ActorBelongsToScene(const Game::Scene& scene, const Game::Actor* actor)
        {
            if (!actor)
                return false;

            const auto& actors = scene.GetActors();
            return std::any_of(actors.begin(), actors.end(), [actor](const auto& candidate)
            {
                return candidate.get() == actor;
            });
        }

        void DrawTransformSection(Game::Actor& actor)
        {
            if (!Utils::BeginCollapsibleSection("Transform"))
                return;

            Math::Vector3 position = actor.GetPosition();
            if (ImGui::DragFloat3("Location", &position.x, 0.1f, 0.0f, 0.0f, "%.3f"))
                actor.SetPosition(position);

            Math::Rotator rotation = actor.GetRotation();
            float rotationValues[3] = { rotation.pitch, rotation.yaw, rotation.roll };
            
            if (ImGui::DragFloat3("Rotation", rotationValues, 0.1f, 0.0f, 0.0f, "%.2f"))
                actor.SetRotation(Math::Rotator(rotationValues[0], rotationValues[1], rotationValues[2]));

            Math::Vector3 scale = actor.GetScale();
            if (ImGui::DragFloat3("Scale", &scale.x, 0.05f, 0.0f, 0.0f, "%.3f"))
                actor.SetScale(scale);
        }

        void DrawComponentSection(const Game::Actor& actor)
        {
            if (!Utils::BeginCollapsibleSection("Components"))
                return;

            const auto& components = actor.GetComponents();
            if (components.empty())
            {
                Utils::DrawEmptyStateMessage("Actor has no components.");
                return;
            }

            ImGui::PushID("ActorComponents");
            for (const auto& component : components)
            {
                if (!component)
                    continue;

                const String typeName = component->GetTypeName();
                const auto typeNameView = typeName.View();

                const bool open = ImGui::TreeNodeEx(
                    component.get(),
                    ImGuiTreeNodeFlags_DefaultOpen |
                    ImGuiTreeNodeFlags_SpanFullWidth |
                    ImGuiTreeNodeFlags_FramePadding,
                    "%.*s",
                    static_cast<int>(typeNameView.size()),
                    typeNameView.data());

                if (open)
                {
                    Utils::DrawEmptyStateMessage("No editable properties available.");
                    ImGui::TreePop();
                }
            }
            ImGui::PopID();
        }
    }

    GuiPanel& CreateActorInspectorPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        GuiPanel& inspectorPanel = guiManager.CreatePanel("actor_inspector", "Details");
        guiManager.SetPanelDockingArea(inspectorPanel, DockSpaceRegion::Right);
        inspectorPanel.SetResizable(true);
        inspectorPanel.SetMovable(true);
        inspectorPanel.SetCollapsable(true);
        inspectorPanel.SetClosable(true);
        inspectorPanel.SetBackgroundColor(kInspectorBackground);
        inspectorPanel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);
        inspectorPanel.SetDrawFunction([provider = context.sceneManagerProvider, getSelectedActor = context.selectedActorGetter, setSelectedActor = context.selectedActorSetter]()
        {
            ImGui::PushID("ActorInspectorPanel");

            static Game::Actor* lastActorForName = nullptr;
            static char nameBuffer[256] = "";

            Game::SceneManager* sceneManager = provider ? provider() : nullptr;
            Game::Scene* activeScene = sceneManager ? sceneManager->GetScene() : nullptr;

            if (!activeScene)
            {
                lastActorForName = nullptr;
                nameBuffer[0] = '\0';
                Utils::DrawEmptyStateMessage("No active scene.");
                ImGui::PopID();
                return;
            }

            Game::Actor* selectedActor = getSelectedActor ? getSelectedActor() : nullptr;
            if (!selectedActor)
            {
                lastActorForName = nullptr;
                nameBuffer[0] = '\0';
                Utils::DrawEmptyStateMessage("No actor selected.");
                ImGui::PopID();
                return;
            }

            if (!ActorBelongsToScene(*activeScene, selectedActor))
            {
                lastActorForName = nullptr;
                nameBuffer[0] = '\0';

                if (setSelectedActor)
                    setSelectedActor(nullptr);

                Utils::DrawEmptyStateMessage("The selected actor is no longer available.");
                ImGui::PopID();
                return;
            }

            if (selectedActor != lastActorForName)
            {
                const String& actorName = selectedActor->GetName();
                const auto actorNameView = actorName.View();
                
                const std::size_t copyLength = std::min(actorNameView.size(), sizeof(nameBuffer) - 1);
                std::memcpy(nameBuffer, actorNameView.data(), copyLength);
                
                nameBuffer[copyLength] = '\0';
                lastActorForName = selectedActor;
            }

            if (Utils::InputTextWithLabel("Name", nameBuffer, IM_ARRAYSIZE(nameBuffer), ImGuiInputTextFlags_None))
                selectedActor->SetName(nameBuffer);

            const String typeName = selectedActor->GetTypeName();
            const auto typeNameView = typeName.View();
            Utils::DrawLabelValue("Type", std::string(typeNameView));

            ImGui::Separator();
            DrawTransformSection(*selectedActor);

            ImGui::Separator();
            DrawComponentSection(*selectedActor);

            ImGui::PopID();
        });

        return inspectorPanel;
    }
}
