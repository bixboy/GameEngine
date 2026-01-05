#include "Gui/Panels/ActorInspector/InspectorSections/ComponentInspectorSection.h"
#include "Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Utils/ContentBrowserUtils.h"
#include "Utils/Editor/ScriptUtils.h"
#include "Framework/Actor.h"
#include "Components/Core/Component.h"
#include "Core/ClassInfo.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <imgui.h>
#include "Utils/ReflectionHelpers.h"
#include "Gui/Panels/ActorInspector/PropertyInspector.h"
#include "Gui/Widgets/Layout/SectionContainer.h"


namespace BixEngine::Gui::ActorInspector
{
    using namespace Theme;
    using namespace Utils;
    using namespace BixEngine::Gui::Widgets;

    namespace
    {
        struct ComponentClassEntry
        {
            const Reflection::ClassInfo* info{nullptr};
            std::string label;
        };

        std::vector<ComponentClassEntry> BuildComponentEntries()
        {
            const auto& componentClass = Game::Component::StaticClass();
            
            std::vector<Reflection::ClassInfo*> allClasses = Reflection::GetAllClasses();

            const std::filesystem::path contentRoot = ContentBrowserUtils::GetContentRoot();
            const std::filesystem::path scriptsDir = contentRoot / "Scripts";
            
            if (std::filesystem::exists(scriptsDir))
            {
                auto scriptRoots = ScriptUtils::Utilities::BuildScriptTree(scriptsDir, contentRoot);
                
                std::function<void(const std::vector<ScriptUtils::ScriptNode>&)> processNodes;
                processNodes = [&](const std::vector<ScriptUtils::ScriptNode>& nodes)
                {
                    for (const auto& node : nodes)
                    {
                        if (node.inheritsComponent)
                        {
                            if (auto* info = Reflection::FindClass(node.name))
                            {
                                allClasses.push_back(info);
                            }
                            else if (auto* infoQ = Reflection::FindClassByQualifiedName(node.name))
                            {
                                allClasses.push_back(infoQ);
                            }
                        }
                        processNodes(node.children);
                    }
                };
                processNodes(scriptRoots);
            }

            std::vector<ComponentClassEntry> entries;
            entries.reserve(allClasses.size());

            std::vector<Reflection::ClassInfo*> processed;
            processed.reserve(allClasses.size());

            for (auto* classInfo : allClasses)
            {
                if (!classInfo || classInfo == &componentClass)
                    continue;
                
                if (classInfo->IsAbstract || !classInfo->CanConstruct())
                    continue;

                if (std::find(processed.begin(), processed.end(), classInfo) != processed.end())
                    continue;

                if (!ScriptUtils::Utilities::IsSubclassOf(*classInfo, componentClass))
                    continue;

                ComponentClassEntry entry{};
                entry.info = classInfo;
                entry.label = !classInfo->Name.empty() ? classInfo->Name : classInfo->QualifiedName;
                
                if (entry.label.empty())
                    entry.label = "Component (Unknown)";

                entries.push_back(std::move(entry));
                processed.push_back(classInfo);
            }

            std::sort(entries.begin(), entries.end(), [](const ComponentClassEntry& lhs, const ComponentClassEntry& rhs)
            {
                return lhs.label < rhs.label;
            });

            return entries;
        }
    }

    void ComponentInspectorSection::DrawAddComponentPopup(Game::Actor& actor)
    {
        if (!ImGui::BeginPopup("AddComponentPopup"))
            return;
        
        static std::vector<ComponentClassEntry> cachedEntries;
        if (ImGui::IsWindowAppearing())
        {
            cachedEntries = BuildComponentEntries();
        }
        
        ImGui::TextUnformatted("Add Component");
        ImGui::Separator();

        static char searchBuffer[64] = "";
        if (ImGui::IsWindowAppearing()) 
            std::memset(searchBuffer, 0, sizeof(searchBuffer));
            
        ImGui::InputTextWithHint("##SearchComp", "Search...", searchBuffer, sizeof(searchBuffer));
        std::string searchKey = searchBuffer;
        std::transform(searchKey.begin(), searchKey.end(), searchKey.begin(), ::tolower);

        if (cachedEntries.empty())
        {
            GuiUtils::DrawEmptyStateMessage("No components available.");
        }
        else
        {
            if (ImGui::BeginChild("ComponentList", ImVec2(0, 250), false, ImGuiWindowFlags_HorizontalScrollbar)) 
            {
                for (const ComponentClassEntry& entry : cachedEntries)
                {
                    if (!entry.info)
                        continue;

                    if (!searchKey.empty())
                    {
                         std::string labelLower = entry.label;
                         std::transform(labelLower.begin(), labelLower.end(), labelLower.begin(), tolower);
                         if (labelLower.find(searchKey) == std::string::npos)
                             continue;
                    }

                    if (ImGui::Selectable(entry.label.c_str()))
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

                    const std::string& tooltip = !entry.info->QualifiedName.empty() ? entry.info->QualifiedName : entry.label;
                    if (ImGui::IsItemHovered())
                         ImGui::SetTooltip("%s", tooltip.c_str());
                }
            }
            ImGui::EndChild();
        }

        ImGui::EndPopup();
    }

    void ComponentInspectorSection::Draw(Game::Actor& actor)
    {
        const std::string contextId = BuildActorContextId(actor);
        PersistentSectionScope section("Components", contextId);
        
        if (!section.IsOpen())
            return;

        Layout::SectionContainer container("ComponentsSection");
        if (!container.IsVisible())
            return;

        GuiUtils::DrawSeparatorText("Components");

        if (ImGui::Button("+ Add Component", ImVec2(-1, 0))) 
        {
            ImGui::OpenPopup("AddComponentPopup");
        }
        
        DrawAddComponentPopup(actor);

        auto& components = actor.GetComponents();
        if (components.empty())
        {
            GuiUtils::DrawEmptyStateMessage("Actor has no components.");
            return;
        }

        GuiUtils::ScopedID componentsId("ActorComponents");
        
        // Styles des headers
        GuiUtils::ScopedColor headerColor(ImGuiCol_Header, ImVec4(0.25f, 0.33f, 0.45f, 0.65f));
        GuiUtils::ScopedColor headerHoverColor(ImGuiCol_HeaderHovered, ImVec4(0.30f, 0.40f, 0.55f, 0.75f));
        GuiUtils::ScopedColor headerActiveColor(ImGuiCol_HeaderActive, ImVec4(0.20f, 0.32f, 0.52f, 0.85f));
        GuiUtils::ScopedStyle framePadding(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));

        static Game::Component* componentPendingRemoval = nullptr;

        for (std::size_t index = 0; index < components.size(); ++index)
        {
            auto& component = components[index];
            if (!component)
                continue;

            GuiUtils::ScopedID componentId(static_cast<int>(index));

            std::string typeLabel;
            const Reflection::ClassInfo& classInfo = component->GetClass();
            
            typeLabel = !classInfo.Name.empty() ? classInfo.Name : (!classInfo.QualifiedName.empty() ? classInfo.QualifiedName : "");
            
            if (typeLabel.empty())
                typeLabel = ToStdString(component->GetTypeName());
            
            const std::string treeLabel = "🧩 " + typeLabel;

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 6.0f));
            bool open = ImGui::CollapsingHeader(treeLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);
            ImGui::PopStyleVar();

            // --- Bouton Supprimer---
            {
                const float buttonSize = ImGui::GetFrameHeight();
                const float availableWidth = ImGui::GetContentRegionAvail().x;
                const float buttonX = ImGui::GetCursorPosX() + availableWidth - buttonSize - 5.0f;
                const float buttonY = ImGui::GetItemRectMin().y; 

                ImVec2 backupCursor = ImGui::GetCursorPos();
                ImGui::SetCursorScreenPos(ImVec2(ImGui::GetWindowPos().x + buttonX, buttonY));
                
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                if (GuiUtils::IconButton("🗑", "Remove this component"))
                {
                    componentPendingRemoval = component.get();
                }
                ImGui::PopStyleColor();
                ImGui::SetCursorPos(backupCursor);
            }

            if (open)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
                ImGui::Indent(10.0f);
                ImGui::Spacing();

                const float cursorBefore = ImGui::GetCursorPosY();
                
                const bool drewReflected = PropertyInspector::DrawClassProperties(component->GetClass(), component.get(), false, nullptr, false);
                const float cursorAfterReflected = ImGui::GetCursorPosY();
                
                component->DrawInspectorUI();
                const float cursorAfterCustom = ImGui::GetCursorPosY();

                if (!drewReflected && cursorAfterCustom <= cursorBefore + 0.5f && cursorAfterReflected <= cursorBefore + 0.5f)
                {
                    GuiUtils::DrawEmptyStateMessage("No editable properties.");
                }

                ImGui::Unindent(10.0f);
                ImGui::PopStyleVar();
                ImGui::Spacing();
            }

            ImGui::Separator();
            ImGui::Spacing();
        }

        if (componentPendingRemoval)
        {
            ImGui::OpenPopup("ConfirmRemoveComponent");
        }

        if (ImGui::BeginPopupModal("ConfirmRemoveComponent", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Remove component '%s'?", componentPendingRemoval->GetClass().Name.c_str());
            ImGui::Separator();

            if (ImGui::Button("Remove", ImVec2(120, 0)))
            {
                actor.RemoveComponent(componentPendingRemoval);
                componentPendingRemoval = nullptr;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                componentPendingRemoval = nullptr;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}
