#include "Bix/Engine/Gui/Panels/ActorInspectorPanel.h"

#include "Bix/Engine/Gui/GuiManager.h"
#include "Bix/Engine/Gui/GuiPanel.h"
#include "Bix/Engine/Gui/Utils/GuiHelpers.h"
#include "Bix/Game/Actor.h"
#include "Bix/Game/Components/Component.h"
#include "Bix/Game/Components/ComponentRegistry.h"
#include "Bix/Game/Scene.h"
#include "Bix/Game/SceneManager.h"
#include "Bix/Math/Vector2.h"
#include "Bix/Math/Vector3.h"
#include "Bix/Math/Rotator.h"
#include "Bix/Reflection/BixReflection.h"

#include "imgui.h"
#include "SDL3/SDL.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cctype>
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

        using ReflectionClass = ::Bix::Reflection::ClassInfo;
        using ReflectionProperty = ::Bix::Reflection::PropertyInfo;

        bool MatchesType(const std::string& typeName, std::string_view expectedSuffix)
        {
            if (typeName.size() < expectedSuffix.size())
            {
                return false;
            }

            const std::size_t offset = typeName.size() - expectedSuffix.size();
            if (typeName.compare(offset, expectedSuffix.size(), expectedSuffix) != 0)
            {
                return false;
            }

            if (offset == 0)
            {
                return true;
            }

            const char preceding = typeName[offset - 1];
            if (std::isalnum(static_cast<unsigned char>(preceding)) != 0 || preceding == '_')
            {
                return false;
            }

            return true;
        }

        std::string MakeDisplayName(const std::string& rawName)
        {
            std::string trimmed = rawName;
            while (!trimmed.empty() && trimmed.back() == '_')
            {
                trimmed.pop_back();
            }

            if (trimmed.empty())
            {
                return "Property";
            }

            std::string result;
            result.reserve(trimmed.size() * 2);

            char previous = '\0';
            for (char ch : trimmed)
            {
                if (ch == '_')
                {
                    if (!result.empty() && result.back() != ' ')
                    {
                        result.push_back(' ');
                    }
                    previous = ch;
                    continue;
                }

                const bool isUpper = std::isupper(static_cast<unsigned char>(ch)) != 0;
                const bool prevLower = std::islower(static_cast<unsigned char>(previous)) != 0;
                const bool isDigit = std::isdigit(static_cast<unsigned char>(ch)) != 0;

                if (!result.empty() && (isUpper && prevLower))
                {
                    result.push_back(' ');
                }
                else if (!result.empty() && isDigit && std::isdigit(static_cast<unsigned char>(previous)) == 0 && !std::isspace(static_cast<unsigned char>(result.back())))
                {
                    result.push_back(' ');
                }

                result.push_back(ch);
                previous = ch;
            }

            if (!result.empty())
            {
                result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
            }

            return result;
        }

        bool DrawSupportedProperty(const ReflectionProperty& property, void* instance, const std::string& label)
        {
            if (!property.IsValid())
            {
                return false;
            }

            if (MatchesType(property.TypeName, "bool"))
            {
                bool& value = property.Get<bool>(instance);
                return ImGui::Checkbox(label.c_str(), &value);
            }

            if (MatchesType(property.TypeName, "int") || MatchesType(property.TypeName, "int32_t") || MatchesType(property.TypeName, "std::int32_t"))
            {
                int& value = property.Get<int>(instance);
                return ImGui::DragInt(label.c_str(), &value, 1.0f);
            }

            if (MatchesType(property.TypeName, "std::int64_t") || MatchesType(property.TypeName, "int64_t"))
            {
                long long& value = property.Get<long long>(instance);
                return ImGui::DragScalar(label.c_str(), ImGuiDataType_S64, &value, 1.0f);
            }

            if (MatchesType(property.TypeName, "unsigned int") || MatchesType(property.TypeName, "uint32_t") || MatchesType(property.TypeName, "std::uint32_t"))
            {
                unsigned int& value = property.Get<unsigned int>(instance);
                return ImGui::DragScalar(label.c_str(), ImGuiDataType_U32, &value, 1.0f);
            }

            if (MatchesType(property.TypeName, "float"))
            {
                float& value = property.Get<float>(instance);
                return ImGui::DragFloat(label.c_str(), &value, 0.1f);
            }

            if (MatchesType(property.TypeName, "double"))
            {
                double& value = property.Get<double>(instance);
                float temp = static_cast<float>(value);
                if (ImGui::DragFloat(label.c_str(), &temp, 0.1f))
                {
                    value = static_cast<double>(temp);
                    return true;
                }
                return false;
            }

            if (MatchesType(property.TypeName, "Math::Vector2") || MatchesType(property.TypeName, "Vector2"))
            {
                Math::Vector2& vector = property.Get<Math::Vector2>(instance);
                float values[2] = { vector.x, vector.y };
                if (ImGui::DragFloat2(label.c_str(), values, 0.1f))
                {
                    vector.x = values[0];
                    vector.y = values[1];
                    return true;
                }
                return false;
            }

            if (MatchesType(property.TypeName, "Math::Vector3") || MatchesType(property.TypeName, "Vector3"))
            {
                Math::Vector3& vector = property.Get<Math::Vector3>(instance);
                float values[3] = { vector.x, vector.y, vector.z };
                if (ImGui::DragFloat3(label.c_str(), values, 0.1f))
                {
                    vector.x = values[0];
                    vector.y = values[1];
                    vector.z = values[2];
                    return true;
                }
                return false;
            }

            if (MatchesType(property.TypeName, "Math::Rotator") || MatchesType(property.TypeName, "Rotator"))
            {
                Math::Rotator& rotator = property.Get<Math::Rotator>(instance);
                float values[3] = { rotator.pitch, rotator.yaw, rotator.roll };
                if (ImGui::DragFloat3(label.c_str(), values, 0.1f))
                {
                    rotator.pitch = values[0];
                    rotator.yaw = values[1];
                    rotator.roll = values[2];
                    return true;
                }
                return false;
            }

            if (MatchesType(property.TypeName, "SDL_Color"))
            {
                SDL_Color& color = property.Get<SDL_Color>(instance);
                float values[4] =
                {
                    static_cast<float>(color.r) / 255.0f,
                    static_cast<float>(color.g) / 255.0f,
                    static_cast<float>(color.b) / 255.0f,
                    static_cast<float>(color.a) / 255.0f,
                };

                if (ImGui::ColorEdit4(label.c_str(), values))
                {
                    color.r = static_cast<Uint8>(std::clamp(values[0], 0.0f, 1.0f) * 255.0f + 0.5f);
                    color.g = static_cast<Uint8>(std::clamp(values[1], 0.0f, 1.0f) * 255.0f + 0.5f);
                    color.b = static_cast<Uint8>(std::clamp(values[2], 0.0f, 1.0f) * 255.0f + 0.5f);
                    color.a = static_cast<Uint8>(std::clamp(values[3], 0.0f, 1.0f) * 255.0f + 0.5f);
                    return true;
                }
                return false;
            }

            if (MatchesType(property.TypeName, "String") || MatchesType(property.TypeName, "std::string"))
            {
                if (MatchesType(property.TypeName, "String"))
                {
                    BixEngine::String& stringValue = property.Get<BixEngine::String>(instance);
                    std::array<char, 512> buffer{};
                    const std::string current = stringValue.Std();
                    const std::size_t copyLength = std::min(buffer.size() - 1, current.size());
                    std::memcpy(buffer.data(), current.data(), copyLength);
                    buffer[copyLength] = '\0';
                    if (ImGui::InputText(label.c_str(), buffer.data(), buffer.size()))
                    {
                        stringValue = buffer.data();
                        return true;
                    }
                    return false;
                }

                std::string& stringValue = property.Get<std::string>(instance);
                std::array<char, 512> buffer{};
                const std::size_t copyLength = std::min(buffer.size() - 1, stringValue.size());
                std::memcpy(buffer.data(), stringValue.data(), copyLength);
                buffer[copyLength] = '\0';
                if (ImGui::InputText(label.c_str(), buffer.data(), buffer.size()))
                {
                    stringValue.assign(buffer.data());
                    return true;
                }
                return false;
            }

            return false;
        }

        void DrawUnsupportedProperty(const ReflectionProperty& property, const std::string& label)
        {
            std::string message = property.TypeName;
            if (!message.empty())
            {
                message += " (read-only)";
            }
            else
            {
                message = "Unsupported";
            }

            Utils::DrawLabelValue(label.c_str(), message, "");
        }

        bool DrawReflectedProperty(const ReflectionProperty& property, void* instance)
        {
            std::string displayName = MakeDisplayName(property.Name);
            if (displayName.empty())
            {
                displayName = property.Name;
            }

            ImGui::PushID(property.Name.c_str());
            const bool handled = DrawSupportedProperty(property, instance, displayName);
            if (!handled)
            {
                DrawUnsupportedProperty(property, displayName);
            }
            ImGui::PopID();
            return handled;
        }

        void GatherClassProperties(const ReflectionClass& classInfo, std::vector<const ReflectionProperty*>& outProperties)
        {
            if (classInfo.SuperClass)
            {
                GatherClassProperties(*classInfo.SuperClass, outProperties);
            }

            for (const auto& property : classInfo.Properties)
            {
                outProperties.push_back(&property);
            }
        }

        bool DrawScriptObjectProperties(Game::Scripting::ScriptBase& scriptObject, bool includeHeader)
        {
            const auto* classInfo = scriptObject.GetScriptClass().GetReflectionInfo();
            if (!classInfo)
            {
                return false;
            }

            std::vector<const ReflectionProperty*> properties;
            properties.reserve(classInfo->Properties.size());
            GatherClassProperties(*classInfo, properties);

            if (properties.empty())
            {
                return false;
            }

            if (includeHeader)
            {
                Utils::DrawSeparatorText("Properties");
            }

            bool anyDrawn = false;
            for (const ReflectionProperty* property : properties)
            {
                if (!property)
                {
                    continue;
                }

                anyDrawn = DrawReflectedProperty(*property, &scriptObject) || anyDrawn;
            }

            return anyDrawn;
        }

        void DrawAddComponentPopup(Game::Actor& actor)
        {
            if (!ImGui::BeginPopup("AddComponentPopup"))
                return;

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
                    std::string label = descriptor.name;
                    std::string moduleName;
                    std::string scriptName;

                    if (descriptor.scriptClass)
                    {
                        const auto& scriptInfo = *descriptor.scriptClass;
                        if (!scriptInfo.displayName.IsEmpty())
                        {
                            label = scriptInfo.displayName.Std();
                        }
                        moduleName = scriptInfo.moduleName.Std();
                        scriptName = scriptInfo.name.Std();
                    }

                    if (ImGui::MenuItem(label.c_str()))
                    {
                        if (descriptor.createFunction)
                        {
                            descriptor.createFunction(actor);
                        }
                        ImGui::CloseCurrentPopup();
                    }

                    if (descriptor.scriptClass && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                    {
                        ImGui::BeginTooltip();
                        if (!moduleName.empty())
                        {
                            ImGui::Text("Module: %s", moduleName.c_str());
                        }
                        if (!scriptName.empty())
                        {
                            ImGui::Text("Class: %s", scriptName.c_str());
                        }
                        ImGui::EndTooltip();
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

            DrawScriptObjectProperties(actor, true);
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

            ImGui::TextUnformatted("Transform");
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
                return;

            SectionContainer container("ComponentsSection");
            if (!container.IsVisible())
                return;

            Utils::DrawSeparatorText("Components");

            if (ImGui::Button("+ Add Component"))
                ImGui::OpenPopup("AddComponentPopup");

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

            static Game::Component* componentPendingRemoval = nullptr;
            for (std::size_t index = 0; index < components.size(); ++index)
            {
                auto& component = components[index];
                if (!component)
                    continue;

                ImGui::PushID(static_cast<int>(index));

                const std::string typeLabel = ToStdString(component->GetTypeName());

                bool open = ImGui::TreeNodeEx(
                    component.get(),
                    ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding,
                    "%s",
                    typeLabel.c_str());

                ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() - ImGui::GetStyle().FramePadding.x);
                if (Utils::IconButton("🗑", "Remove this component"))
                    componentPendingRemoval = component.get();

                if (open)
                {
                    const float startCursor = ImGui::GetCursorPosY();
                    component->DrawInspectorUI();
                    DrawScriptObjectProperties(*component, false);
                    const float endCursor = ImGui::GetCursorPosY();

                    if (endCursor <= startCursor + FLT_EPSILON)
                        Utils::DrawEmptyStateMessage("No editable properties.");
                    
                    ImGui::TreePop();
                }

                ImGui::PopID();
            }

            ImGui::PopID();

            // --- popup modale de confirmation ---
            if (componentPendingRemoval)
                ImGui::OpenPopup("ConfirmRemoveComponent");

            if (ImGui::BeginPopupModal("ConfirmRemoveComponent", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextUnformatted("Are you sure you want to remove this component?");
                ImGui::Separator();

                if (ImGui::Button("Remove"))
                {
                    actor.RemoveComponent(componentPendingRemoval);
                    componentPendingRemoval = nullptr;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                if (ImGui::Button("Cancel"))
                {
                    componentPendingRemoval = nullptr;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
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
