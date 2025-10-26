#include "Engine/Gui/Panels/ActorInspector/ComponentSectionUI.h"

#include "Engine/Gui/Panels/ActorInspector/ImGuiControls.h"
#include "Engine/Gui/Panels/ActorInspector/ReflectionDrawer.h"
#include "Engine/Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"

#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Game/Actor.h"
#include "Game/Components/Component.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <imgui.h>

namespace BixEngine::Gui::ActorInspector
{
    void DrawAddComponentPopup(Game::Actor& actor)
    {
        if (!ImGui::BeginPopup("AddComponentPopup"))
        {
            return;
        }

        ImGui::TextUnformatted("Add Component");
        ImGui::Separator();

        const auto& componentClass = Game::Component::StaticClass();

        struct ComponentClassEntry
        {
            const ::Bix::Reflection::ClassInfo* info{nullptr};
            std::string label;
        };

        std::vector<ComponentClassEntry> entries;
        entries.reserve(32);

        for (auto* classInfo : ::Bix::Reflection::GetAllClasses())
        {
            if (!classInfo || classInfo == &componentClass)
            {
                continue;
            }

            if (!IsSubclassOf(*classInfo, componentClass))
            {
                continue;
            }

            if (classInfo->IsAbstract || !classInfo->CanConstruct())
            {
                continue;
            }

            ComponentClassEntry entry{};
            entry.info = classInfo;
            entry.label = !classInfo->Name.empty() ? classInfo->Name : classInfo->QualifiedName;
            if (entry.label.empty())
            {
                entry.label = "Component";
            }

            entries.push_back(std::move(entry));
        }

        std::sort(entries.begin(), entries.end(), [](const ComponentClassEntry& lhs, const ComponentClassEntry& rhs)
        {
            return lhs.label < rhs.label;
        });

        if (entries.empty())
        {
            Utils::DrawEmptyStateMessage("No components available.");
        }
        else
        {
            for (const ComponentClassEntry& entry : entries)
            {
                if (!entry.info)
                {
                    continue;
                }

                if (ImGui::MenuItem(entry.label.c_str()))
                {
                    std::unique_ptr<Game::Component> ownedComponent;
                    if (void* instance = entry.info->Construct(&actor))
                    {
                        ownedComponent.reset(static_cast<Game::Component*>(instance));
                    }

                    if (ownedComponent)
                    {
                        actor.AddComponent(std::move(ownedComponent));
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
                {
                    ImGui::BeginTooltip();
                    if (!entry.info->QualifiedName.empty())
                    {
                        ImGui::TextUnformatted(entry.info->QualifiedName.c_str());
                    }
                    else
                    {
                        ImGui::TextUnformatted(entry.label.c_str());
                    }
                    ImGui::EndTooltip();
                }
            }
        }

        ImGui::EndPopup();
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
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.25f, 0.33f, 0.45f, 0.65f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.30f, 0.40f, 0.55f, 0.75f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.20f, 0.32f, 0.52f, 0.85f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));

        static Game::Component* componentPendingRemoval = nullptr;
        for (std::size_t index = 0; index < components.size(); ++index)
        {
            auto& component = components[index];
            if (!component)
            {
                continue;
            }

            ImGui::PushID(static_cast<int>(index));

            const std::string typeLabel = ToStdString(component->GetTypeName());
            const std::string treeLabel = "🧩 " + typeLabel;
            const bool open = ImGui::TreeNodeEx(
                component.get(),
                ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding,
                "%s",
                treeLabel.c_str());

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() - ImGui::GetStyle().FramePadding.x);
            if (Utils::IconButton("🗑", "Remove this component"))
            {
                componentPendingRemoval = component.get();
            }

            if (open)
            {
                DrawClassProperties(component->GetClass(), component.get(), false, nullptr, true);
                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        if (componentPendingRemoval)
        {
            ImGui::OpenPopup("ConfirmRemoveComponent");
        }

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
}

