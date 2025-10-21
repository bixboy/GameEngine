#include "Bix/Engine/Gui/Panels/ActorInspectorPanel.h"

#include "Bix/Engine/Gui/GuiManager.h"
#include "Bix/Engine/Gui/GuiPanel.h"
#include "Bix/Game/Actor.h"
#include "Bix/Game/Components/Component.h"
#include "Bix/Game/Scene.h"
#include "Bix/Game/SceneManager.h"
#include "Bix/Math/Math.h"

#include "imgui.h"

#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace BixEngine::Gui
{
    namespace
    {
        constexpr ImVec4 kInspectorBackground{0.12f, 0.12f, 0.12f, 0.95f};
        constexpr ImVec4 kDirtyColor{0.25f, 0.35f, 0.65f, 0.45f};
        constexpr ImVec4 kDirtyColorHovered{0.30f, 0.40f, 0.75f, 0.55f};
        constexpr ImVec4 kDirtyColorActive{0.35f, 0.45f, 0.85f, 0.65f};

        // ---------------------------------------------------------------------
        // Fake component implementations for demonstration purposes.
        // In a production engine these would live in their own translation units
        // and expose real runtime reflection metadata.
        // ---------------------------------------------------------------------
        class MeshComponent final : public Game::Component
        {
        public:
            explicit MeshComponent(Game::Actor* owner)
                : Game::Component(owner)
            {
            }

            [[nodiscard]] String GetTypeName() const override { return "MeshComponent"; }

            float tessellationFactor{1.0f};
            bool castsShadow{true};
        };

        class LightComponent final : public Game::Component
        {
        public:
            explicit LightComponent(Game::Actor* owner)
                : Game::Component(owner)
            {
            }

            [[nodiscard]] String GetTypeName() const override { return "LightComponent"; }

            Math::Vector3 color{1.0f, 1.0f, 1.0f};
            float intensity{10.0f};
            Game::Actor* targetActor{nullptr};
        };

        class ColliderComponent final : public Game::Component
        {
        public:
            explicit ColliderComponent(Game::Actor* owner)
                : Game::Component(owner)
            {
            }

            [[nodiscard]] String GetTypeName() const override { return "ColliderComponent"; }

            Math::Vector3 extents{50.0f, 50.0f, 50.0f};
            bool isTrigger{false};
        };

        // ---------------------------------------------------------------------
        // Dirty state tracking
        // ---------------------------------------------------------------------
        struct TransformDirtyState
        {
            bool position{false};
            bool rotation{false};
            bool scale{false};
        };

        struct ComponentDirtyState
        {
            std::unordered_map<std::string, bool> propertyStates{};
        };

        static std::unordered_map<Game::Actor*, TransformDirtyState> g_transformDirtyStates;
        static std::unordered_map<Game::Component*, ComponentDirtyState> g_componentDirtyStates;

        void PushDirtyStyle()
        {
            ImGui::PushStyleColor(ImGuiCol_FrameBg, kDirtyColor);
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, kDirtyColorHovered);
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, kDirtyColorActive);
        }

        void PopDirtyStyle()
        {
            ImGui::PopStyleColor(3);
        }

        // ---------------------------------------------------------------------
        // Fake reflection helpers
        // ---------------------------------------------------------------------
        struct PropertyEntry
        {
            enum class Kind
            {
                Float,
                Bool,
                Vector3,
                ActorReference,
            };

            std::string label;
            Kind kind{Kind::Float};
            std::function<void(Game::Component&)> drawer{};
        };

        class FakeReflectionRegistry
        {
        public:
            using PropertyList = std::vector<PropertyEntry>;

            static FakeReflectionRegistry& Instance()
            {
                static FakeReflectionRegistry instance;
                return instance;
            }

            template <typename ComponentType>
            void RegisterFloat(const char* label, float ComponentType::*member)
            {
                PropertyEntry entry;
                entry.label = label;
                entry.kind = PropertyEntry::Kind::Float;
                entry.drawer = [label, member](Game::Component& component)
                {
                    auto& typed = static_cast<ComponentType&>(component);
                    float& value = typed.*member;

                    auto& dirty = g_componentDirtyStates[&component].propertyStates[label];
                    if (dirty)
                        PushDirtyStyle();

                    if (ImGui::DragFloat(label, &value, 0.1f, 0.0f, 0.0f, "%.3f"))
                        dirty = true;

                    if (dirty)
                        PopDirtyStyle();
                };
                properties_[std::type_index(typeid(ComponentType))].push_back(std::move(entry));
            }

            template <typename ComponentType>
            void RegisterBool(const char* label, bool ComponentType::*member)
            {
                PropertyEntry entry;
                entry.label = label;
                entry.kind = PropertyEntry::Kind::Bool;
                entry.drawer = [label, member](Game::Component& component)
                {
                    auto& typed = static_cast<ComponentType&>(component);
                    bool& value = typed.*member;

                    auto& dirty = g_componentDirtyStates[&component].propertyStates[label];
                    if (dirty)
                        PushDirtyStyle();

                    if (ImGui::Checkbox(label, &value))
                        dirty = true;

                    if (dirty)
                        PopDirtyStyle();
                };
                properties_[std::type_index(typeid(ComponentType))].push_back(std::move(entry));
            }

            template <typename ComponentType>
            void RegisterVector3(const char* label, Math::Vector3 ComponentType::*member)
            {
                PropertyEntry entry;
                entry.label = label;
                entry.kind = PropertyEntry::Kind::Vector3;
                entry.drawer = [label, member](Game::Component& component)
                {
                    auto& typed = static_cast<ComponentType&>(component);
                    Math::Vector3& value = typed.*member;
                    auto& dirty = g_componentDirtyStates[&component].propertyStates[label];

                    float values[3] = { value.x, value.y, value.z };
                    if (dirty)
                        PushDirtyStyle();

                    if (ImGui::DragFloat3(label, values, 0.1f, 0.0f, 0.0f, "%.3f"))
                    {
                        value.x = values[0];
                        value.y = values[1];
                        value.z = values[2];
                        dirty = true;
                    }

                    if (dirty)
                        PopDirtyStyle();
                };
                properties_[std::type_index(typeid(ComponentType))].push_back(std::move(entry));
            }

            template <typename ComponentType>
            void RegisterActorReference(const char* label, Game::Actor* ComponentType::*member)
            {
                PropertyEntry entry;
                entry.label = label;
                entry.kind = PropertyEntry::Kind::ActorReference;
                entry.drawer = [label, member](Game::Component& component)
                {
                    auto& typed = static_cast<ComponentType&>(component);
                    Game::Actor*& target = typed.*member;

                    auto& dirty = g_componentDirtyStates[&component].propertyStates[label];

                    if (dirty)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, kDirtyColor);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kDirtyColorHovered);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, kDirtyColorActive);
                    }

                    const char* buttonLabel = target ? target->GetName().c_str() : "<None>";
                    std::string formattedLabel = label;
                    formattedLabel += ": ";
                    formattedLabel += buttonLabel;

                    if (ImGui::Button(formattedLabel.c_str(), ImVec2(-FLT_MIN, 0.0f)))
                    {
                        // Reserve for future selection UI.
                    }

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ACTOR_ID"))
                        {
                            if (payload->Data && payload->DataSize == sizeof(Game::Actor*))
                            {
                                auto droppedActor = *static_cast<Game::Actor* const*>(payload->Data);
                                target = droppedActor;
                                dirty = true;
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }

                    if (dirty)
                        ImGui::PopStyleColor(3);
                };
                properties_[std::type_index(typeid(ComponentType))].push_back(std::move(entry));
            }

            [[nodiscard]] const PropertyList* Find(const Game::Component& component) const
            {
                const auto it = properties_.find(std::type_index(typeid(component)));
                if (it == properties_.end())
                    return nullptr;
                return &it->second;
            }

        private:
            FakeReflectionRegistry()
            {
                RegisterFloat<MeshComponent>("Tessellation", &MeshComponent::tessellationFactor);
                RegisterBool<MeshComponent>("Casts Shadow", &MeshComponent::castsShadow);

                RegisterVector3<LightComponent>("Color", &LightComponent::color);
                RegisterFloat<LightComponent>("Intensity", &LightComponent::intensity);
                RegisterActorReference<LightComponent>("Target Actor", &LightComponent::targetActor);

                RegisterVector3<ColliderComponent>("Extents", &ColliderComponent::extents);
                RegisterBool<ColliderComponent>("Is Trigger", &ColliderComponent::isTrigger);
            }

            std::unordered_map<std::type_index, PropertyList> properties_{};
        };

        struct AvailableComponent
        {
            const char* label;
            std::function<std::unique_ptr<Game::Component>(Game::Actor&)> factory;
        };

        std::vector<AvailableComponent> BuildAvailableComponents()
        {
            return {
                AvailableComponent{"MeshComponent", [](Game::Actor& actor)
                {
                    return std::make_unique<MeshComponent>(&actor);
                }},
                AvailableComponent{"LightComponent", [](Game::Actor& actor)
                {
                    return std::make_unique<LightComponent>(&actor);
                }},
                AvailableComponent{"ColliderComponent", [](Game::Actor& actor)
                {
                    return std::make_unique<ColliderComponent>(&actor);
                }},
            };
        }

        const std::vector<AvailableComponent>& GetAvailableComponents()
        {
            static const std::vector<AvailableComponent> components = BuildAvailableComponents();
            return components;
        }

        FakeReflectionRegistry& GetReflectionRegistry()
        {
            return FakeReflectionRegistry::Instance();
        }

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

        void ResetTransform(Game::Actor& actor)
        {
            actor.SetPosition({0.0f, 0.0f, 0.0f});
            actor.SetRotation(Math::Rotator(0.0f, 0.0f, 0.0f));
            actor.SetScale({1.0f, 1.0f, 1.0f});

            auto& state = g_transformDirtyStates[&actor];
            state.position = false;
            state.rotation = false;
            state.scale = false;
        }

        void DrawTransformSection(Game::Actor& actor)
        {
            TransformDirtyState& dirtyState = g_transformDirtyStates[&actor];

            const ImGuiTreeNodeFlags headerFlags =
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_FramePadding;

            ImGui::PushID(&actor);
            const bool open = ImGui::TreeNodeEx("TransformSection", headerFlags, "Transform");

            const ImGuiStyle& style = ImGui::GetStyle();
            const float buttonWidth = ImGui::CalcTextSize("Reset Transform").x + style.FramePadding.x * 2.0f;
            ImGui::SameLine();
            const float cursorStart = ImGui::GetCursorPosX();
            const float availWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(cursorStart + std::max(0.0f, availWidth - buttonWidth));
            if (ImGui::SmallButton("Reset Transform"))
                ResetTransform(actor);

            if (open)
            {
                Math::Vector3 position = actor.GetPosition();
                float positionValues[3] = { position.x, position.y, position.z };
                if (dirtyState.position)
                    PushDirtyStyle();
                if (ImGui::DragFloat3("Location", positionValues, 0.1f, 0.0f, 0.0f, "%.3f"))
                {
                    actor.SetPosition({positionValues[0], positionValues[1], positionValues[2]});
                    dirtyState.position = true;
                }
                if (dirtyState.position)
                    PopDirtyStyle();

                Math::Rotator rotation = actor.GetRotation();
                float rotationValues[3] = { rotation.pitch, rotation.yaw, rotation.roll };
                if (dirtyState.rotation)
                    PushDirtyStyle();
                if (ImGui::DragFloat3("Rotation", rotationValues, 0.1f, -360.0f, 360.0f, "%.2f"))
                {
                    actor.SetRotation(Math::Rotator(rotationValues[0], rotationValues[1], rotationValues[2]));
                    dirtyState.rotation = true;
                }
                if (dirtyState.rotation)
                    PopDirtyStyle();

                Math::Vector3 scale = actor.GetScale();
                float scaleValues[3] = { scale.x, scale.y, scale.z };
                if (dirtyState.scale)
                    PushDirtyStyle();
                if (ImGui::DragFloat3("Scale", scaleValues, 0.05f, 0.0f, 0.0f, "%.3f"))
                {
                    actor.SetScale({scaleValues[0], scaleValues[1], scaleValues[2]});
                    dirtyState.scale = true;
                }
                if (dirtyState.scale)
                    PopDirtyStyle();

                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        void DrawComponentProperties(Game::Component& component)
        {
            const FakeReflectionRegistry::PropertyList* properties = GetReflectionRegistry().Find(component);
            if (!properties || properties->empty())
            {
                ImGui::TextDisabled("No editable properties available.");
                return;
            }

            for (const PropertyEntry& property : *properties)
            {
                if (property.drawer)
                    property.drawer(component);
            }
        }

        void DrawComponentSection(Game::Actor& actor)
        {
            ImGui::SeparatorText("Components");
            ImGui::SameLine();
            const ImGuiStyle& style = ImGui::GetStyle();
            const float addButtonWidth = ImGui::CalcTextSize("+ Add Component").x + style.FramePadding.x * 2.0f;
            const float buttonCursorStart = ImGui::GetCursorPosX();
            const float buttonAvail = ImGui::GetContentRegionAvail().x;
            ImGui::SetCursorPosX(buttonCursorStart + std::max(0.0f, buttonAvail - addButtonWidth));
            if (ImGui::Button("+ Add Component"))
                ImGui::OpenPopup("AddComponentPopup");

            if (ImGui::BeginPopup("AddComponentPopup"))
            {
                for (const AvailableComponent& descriptor : GetAvailableComponents())
                {
                    if (ImGui::Selectable(descriptor.label))
                    {
                        if (descriptor.factory)
                            actor.AddComponent(descriptor.factory(actor));
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }

            const auto& components = actor.GetComponents();
            if (components.empty())
            {
                ImGui::Spacing();
                ImGui::TextDisabled("Actor has no components.");
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
                    ImGuiTreeNodeFlags_SpanAvailWidth |
                    ImGuiTreeNodeFlags_Framed |
                    ImGuiTreeNodeFlags_FramePadding,
                    "%.*s",
                    static_cast<int>(typeNameView.size()),
                    typeNameView.data());

                if (open)
                {
                    DrawComponentProperties(*component);
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
        inspectorPanel.SetDrawFunction([provider = context.sceneManagerProvider, getSelectedActor = context.selectedActorGetter]()
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
                ImGui::TextDisabled("No active scene.");
                ImGui::PopID();
                return;
            }

            Game::Actor* selectedActor = getSelectedActor ? getSelectedActor() : nullptr;
            if (!selectedActor)
            {
                lastActorForName = nullptr;
                nameBuffer[0] = '\0';
                ImGui::TextDisabled("No actor selected.");
                ImGui::PopID();
                return;
            }

            if (!ActorBelongsToScene(*activeScene, selectedActor))
            {
                lastActorForName = nullptr;
                nameBuffer[0] = '\0';
                ImGui::TextDisabled("Actor no longer valid.");
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

            if (ImGui::InputText("Name", nameBuffer, IM_ARRAYSIZE(nameBuffer)))
                selectedActor->SetName(nameBuffer);

            ImGui::SameLine();
            if (ImGui::Button("Focus Actor"))
            {
                // Placeholder for camera focus functionality.
            }

            const String typeName = selectedActor->GetTypeName();
            const auto typeNameView = typeName.View();

            ImGui::TextDisabled("Type: %.*s",
                static_cast<int>(typeNameView.size()),
                typeNameView.data());

            DrawTransformSection(*selectedActor);

            ImGui::Separator();
            DrawComponentSection(*selectedActor);

            ImGui::PopID();
        });

        return inspectorPanel;
    }
}
