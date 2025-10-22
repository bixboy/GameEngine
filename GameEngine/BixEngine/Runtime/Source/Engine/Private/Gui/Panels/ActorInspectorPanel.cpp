#include "Bix/Engine/Gui/Panels/ActorInspectorPanel.h"

#include "Bix/Engine/Gui/GuiManager.h"
#include "Bix/Engine/Gui/GuiPanel.h"
#include "Bix/Engine/Gui/Utils/GuiHelpers.h"
#include "Bix/Core/Logger.h"
#include "Bix/Game/Actor.h"
#include "Bix/Game/Components/Component.h"
#include "Bix/Game/Components/ComponentRegistry.h"
#include "Bix/Game/Scene.h"
#include "Bix/Game/SceneManager.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstring>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace BixEngine::Gui
{
    namespace
    {
        constexpr ImVec4 kInspectorBackground{0.12f, 0.12f, 0.12f, 0.95f};
        constexpr ImVec4 kSectionBackground{0.18f, 0.18f, 0.18f, 0.65f};

        namespace Utils = BixEngine::Gui::Utils;

        struct ActorInspectorState
        {
            std::array<char, 256> nameBuffer{};
        };

        std::unordered_map<std::string, ActorInspectorState>& GetActorStates()
        {
            static std::unordered_map<std::string, ActorInspectorState> states;
            return states;
        }

        std::string ToStdString(const String& value)
        {
            const auto view = value.View();
            return std::string(view.data(), view.size());
        }

        std::string BuildActorContextId(const Game::Actor& actor)
        {
            const auto nameView = actor.GetName().View();
            return std::string(nameView.data(), nameView.size());
        }

        ActorInspectorState& GetActorState(const Game::Actor& actor)
        {
            auto& states = GetActorStates();
            const std::string key = ToStdString(actor.GetUUID());
            auto [it, inserted] = states.try_emplace(key);

            auto& state = it->second;
            if (inserted)
            {
                state.nameBuffer.fill('\0');
            }

            const auto nameView = actor.GetName().View();
            const std::size_t copyLength = std::min<std::size_t>(nameView.size(), state.nameBuffer.size() - 1);
            if (std::strncmp(state.nameBuffer.data(), nameView.data(), copyLength) != 0 || state.nameBuffer[copyLength] != '\0')
            {
                std::memcpy(state.nameBuffer.data(), nameView.data(), copyLength);
                state.nameBuffer[copyLength] = '\0';
            }

            return state;
        }

        class PersistentSectionScope
        {
        public:
            PersistentSectionScope(const char* label, const std::string& contextId, bool defaultOpen = true, ImGuiTreeNodeFlags flags = 0)
                : open_(Utils::BeginPersistentSection(label, contextId, defaultOpen, flags))
            {
            }

            ~PersistentSectionScope()
            {
                if (open_)
                {
                    Utils::EndPersistentSection();
                }
            }

            [[nodiscard]] bool IsOpen() const noexcept { return open_; }

        private:
            bool open_{false};
        };

        class SectionContainer
        {
        public:
            explicit SectionContainer(const char* id)
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, kSectionBackground);
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
                visible_ = ImGui::BeginChild(id, ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar);
            }

            ~SectionContainer()
            {
                ImGui::EndChild();
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor();
                ImGui::Spacing();
            }

            [[nodiscard]] bool IsVisible() const noexcept { return visible_; }

        private:
            bool visible_{false};
        };

        void DrawAddComponentPopup(Game::Actor& actor)
        {
            if (!ImGui::BeginPopup("AddComponentPopup"))
            {
                return;
            }

            ImGui::TextUnformatted("Add Component");
            ImGui::Separator();

            const auto descriptors = Game::ComponentRegistry::GetInstance().GetRegisteredComponents();
            if (descriptors.empty())
            {
                Utils::DrawEmptyStateMessage("No components available.");
            }
            else
            {
                for (const Game::ComponentDescriptor& descriptor : descriptors)
                {
                    if (ImGui::MenuItem(descriptor.name.c_str()))
                    {
                        if (descriptor.createFunction)
                        {
                            descriptor.createFunction(actor);
                        }
                        ImGui::CloseCurrentPopup();
                    }
                }
            }

            ImGui::EndPopup();
        }

        void DrawGeneralSection(Game::Actor& actor)
        {
            const std::string contextId = BuildActorContextId(actor);
            PersistentSectionScope section("General", contextId);
            if (!section.IsOpen())
            {
                return;
            }

            SectionContainer container("GeneralSection");
            if (!container.IsVisible())
            {
                return;
            }

            ActorInspectorState& state = GetActorState(actor);
            if (Utils::InputTextWithLabel("Name", state.nameBuffer.data(), state.nameBuffer.size()))
            {
                actor.SetName(state.nameBuffer.data());
            }

            bool isActive = actor.IsActive();
            if (ImGui::Checkbox("Active", &isActive))
            {
                actor.SetActive(isActive);
            }

            Utils::DrawLabelValue("Type", ToStdString(actor.GetTypeName()));
            Utils::DrawLabelValue("ID", ToStdString(actor.GetUUID()));
        }

        void DrawTransformSection(Game::Actor& actor)
        {
            const std::string contextId = BuildActorContextId(actor);
            PersistentSectionScope section("Transform", contextId);
            if (!section.IsOpen())
            {
                return;
            }

            SectionContainer container("TransformSection");
            if (!container.IsVisible())
            {
                return;
            }

            Utils::DrawSeparatorText("Transform");

            ImGui::TextUnformatted("🧭 Transform");
            ImGui::SameLine();
            Utils::DrawHelpMarker("Adjust the actor position, rotation, and scale.");

            Math::Vector3 position = actor.GetPosition();
            if (ImGui::DragFloat3("Location", &position.x, 0.1f, -FLT_MAX, FLT_MAX, "%.3f"))
            {
                actor.SetPosition(position);
            }

            Math::Rotator rotation = actor.GetRotation();
            float rotationValues[3] = { rotation.pitch, rotation.yaw, rotation.roll };
            if (ImGui::DragFloat3("Rotation", rotationValues, 0.1f, -360.0f, 360.0f, "%.2f"))
            {
                actor.SetRotation(Math::Rotator(rotationValues[0], rotationValues[1], rotationValues[2]));
            }

            Math::Vector3 scale = actor.GetScale();
            if (ImGui::DragFloat3("Scale", &scale.x, 0.05f, 0.0f, 0.0f, "%.3f"))
            {
                actor.SetScale(scale);
            }
        }

        void DrawComponentSection(Game::Actor& actor)
        {
            const std::string contextId = BuildActorContextId(actor);
            PersistentSectionScope section("Components", contextId);
            if (!section.IsOpen())
            {
                return;
            }

            SectionContainer container("ComponentsSection");
            if (!container.IsVisible())
            {
                return;
            }

            Utils::DrawSeparatorText("Components");

            if (ImGui::Button("+ Add Component"))
            {
                ImGui::OpenPopup("AddComponentPopup");
            }
            ImGui::SameLine();
            Utils::DrawHelpMarker("Attach new behaviours to this actor.");
            DrawAddComponentPopup(actor);

            auto& components = actor.GetComponents();
            if (components.empty())
            {
                Utils::DrawEmptyStateMessage("Actor has no components.");
                return;
            }
            ImGui::PushID("ActorComponents");
            const Game::Component* componentToRemove = nullptr;

            for (std::size_t index = 0; index < components.size(); ++index)
            {
                auto& component = components[index];
                if (!component)
                {
                    continue;
                }

                ImGui::PushID(static_cast<int>(index));

                const std::string typeLabel = ToStdString(component->GetTypeName());
                const bool open = ImGui::TreeNodeEx(
                    component.get(),
                    ImGuiTreeNodeFlags_DefaultOpen |
                    ImGuiTreeNodeFlags_SpanFullWidth |
                    ImGuiTreeNodeFlags_FramePadding,
                    "%s",
                    typeLabel.c_str());

                const ImVec2 headerMin = ImGui::GetItemRectMin();
                const ImVec2 headerMax = ImGui::GetItemRectMax();
                const float buttonSize = ImGui::GetFrameHeight();
                const float buttonYOffset = (headerMax.y - headerMin.y - buttonSize) * 0.5f;
                const ImVec2 previousCursor = ImGui::GetCursorScreenPos();

                ImVec2 buttonPosition = ImVec2(headerMax.x - buttonSize - ImGui::GetStyle().FramePadding.x, headerMin.y + buttonYOffset);
                ImGui::SetCursorScreenPos(buttonPosition);
                if (Utils::IconButton("🗑", "Remove component"))
                {
                    componentToRemove = component.get();
                }
                ImGui::SetCursorScreenPos(previousCursor);
                ImGui::Dummy(ImVec2(0.0f, 0.0f));

                if (open)
                {
                    const float startCursor = ImGui::GetCursorPosY();
                    component->DrawInspectorUI();
                    const float endCursor = ImGui::GetCursorPosY();
                    if (endCursor <= startCursor + FLT_EPSILON)
                    {
                        Utils::DrawEmptyStateMessage("No editable properties.");
                    }
                    ImGui::TreePop();
                }

                ImGui::PopID();

                if (componentToRemove)
                {
                    break;
                }
            }

            ImGui::PopID();

            if (componentToRemove)
            {
                if (!actor.RemoveComponent(componentToRemove))
                {
                    LOG_WARNING("Failed to remove component from actor: " + ToStdString(actor.GetName()));
                }
            }
        }

        bool ActorBelongsToScene(const Game::Scene& scene, const Game::Actor* actor)
        {
            if (!actor)
            {
                return false;
            }

            const auto& actors = scene.GetActors();
            return std::any_of(actors.begin(), actors.end(), [actor](const auto& candidate)
            {
                return candidate.get() == actor;
            });
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

            if (!ActorBelongsToScene(*activeScene, selectedActor))
            {
                if (setSelectedActor)
                {
                    setSelectedActor(nullptr);
                }

                Utils::DrawEmptyStateMessage("The selected actor is no longer available.");
                ImGui::PopID();
                return;
            }

            DrawGeneralSection(*selectedActor);
            DrawTransformSection(*selectedActor);
            DrawComponentSection(*selectedActor);

            ImGui::PopID();
        });

        return inspectorPanel;
    }
}
