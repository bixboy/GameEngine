#include "Gui/Panels/ActorInspector/InspectorSections/ComponentInspectorSection.h"
#include "Gui/Widgets/Widgets.h"
#include "Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Utils/ContentBrowserUtils.h"
#include "Utils/ScriptUtils.h"
#include "Actor.h"
#include "Components/Component.h"
#include "ClassInfo.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <imgui.h>
#include "ReflectionHelpers.h"
#include "Gui/Panels/ActorInspector/PropertyInspector.h"
#include "Utils/ScriptUtils.h"


namespace BixEngine::Gui::ActorInspector
{
    using namespace Theme;
    using namespace Utils;
    using namespace BixEngine::Gui::Widgets;

    namespace
    {
        struct ComponentClassEntry
        {
            const Bix::Reflection::ClassInfo* info{nullptr};
            std::string label;
        };

        std::vector<ComponentClassEntry> BuildComponentEntries()
        {
            const auto& componentClass = Game::Component::StaticClass();

            // 1. Get all classes via reflection (current method)
            std::vector<Bix::Reflection::ClassInfo*> allClasses = Bix::Reflection::GetAllClasses();

            // 2. Also retrieve via script parsing to ensure nothing is missed
            // (Sometimes reflection doesn't have the correct SuperClass link yet if loading order is specific)
            const std::filesystem::path contentRoot = ContentBrowserUtils::GetContentRoot();
            const std::filesystem::path scriptsDir = contentRoot / "Scripts";
            
            if (std::filesystem::exists(scriptsDir))
            {
                auto scriptRoots = ScriptUtils::Utilities::BuildScriptTree(scriptsDir, contentRoot);
                
                // Recursive function to traverse the tree
                std::function<void(const std::vector<ScriptUtils::ScriptNode>&)> processNodes;
                processNodes = [&](const std::vector<ScriptUtils::ScriptNode>& nodes)
                {
                    for (const auto& node : nodes)
                    {
                        // If the script says it inherits from Component
                        if (node.inheritsComponent)
                        {
                            // Try to find the class in the registry
                            if (auto* info = Bix::Reflection::FindClass(node.name))
                            {
                                allClasses.push_back(info);
                            }
                            else if (auto* infoQ = Bix::Reflection::FindClassByQualifiedName(node.name))
                            {
                                allClasses.push_back(infoQ);
                            }
                            // If class not found (not compiled/loaded yet?), we can't do more here.
                        }
                        
                        processNodes(node.children);
                    }
                };
                
                processNodes(scriptRoots);
            }

            // 3. Build final list with deduplication
            std::vector<ComponentClassEntry> entries;
            entries.reserve(allClasses.size());

            // For deduplication
            std::vector<Bix::Reflection::ClassInfo*> processed;
            processed.reserve(allClasses.size());

            for (auto* classInfo : allClasses)
            {
                if (!classInfo || classInfo == &componentClass)
                {
                    continue;
                }

                // Check if already processed
                if (std::find(processed.begin(), processed.end(), classInfo) != processed.end())
                {
                    continue;
                }

                // Check inheritance:
                // Either via IsSubclassOf (pure reflection)
                // Or we added it manually because the script said inheritsComponent (so we trust it)
                // But to be safe, we keep IsSubclassOf as main filter, 
                // UNLESS we really want to force everything that looks like a component.
                // The user's problem is that "just a selection" is proposed.
                // If IsSubclassOf fails (missing link), we risk filtering it out.
                // BUT, if we found it via script parsing, we should accept it.
                
                bool isComponent = false;
                if (ScriptUtils::Utilities::IsSubclassOf(*classInfo, componentClass))
                {
                    isComponent = true;
                }
                else
                {
                    // Fallback: check if name matches a known component script?
                    // This is implicitly done because we added to 'allClasses' those that are components according to script.
                    // So if we are here, it is either a valid component by reflection, or one added by our scan.
                }
                
                if (ScriptUtils::Utilities::IsSubclassOf(*classInfo, componentClass))
                {
                    isComponent = true;
                }
                
                if (!isComponent)
                {
                     // Check if explicitly found in scripts as component
                     // (This requires keeping track of script names found)
                     // Let's assume IsSubclassOf is correct for now.
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
                processed.push_back(classInfo);
            }
            
            // Now add those found by script scan that would have been missed by IsSubclassOf
            // (Rare case of incomplete reflection)
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
                             Bix::Reflection::ClassInfo* info = Bix::Reflection::FindClass(node.name);
                             if (!info) info = Bix::Reflection::FindClassByQualifiedName(node.name);
                             
                             if (info && info != &componentClass)
                             {
                                 // If not already processed
                                 if (std::find(processed.begin(), processed.end(), info) == processed.end())
                                 {
                                     // Add it even if IsSubclassOf said no (because script says yes)
                                     if (!info->IsAbstract && info->CanConstruct())
                                     {
                                         ComponentClassEntry entry{};
                                         entry.info = info;
                                         entry.label = !info->Name.empty() ? info->Name : info->QualifiedName;
                                         entries.push_back(std::move(entry));
                                         processed.push_back(info);
                                     }
                                 }
                             }
                         }
                         processNodes(node.children);
                     }
                 };
                 processNodes(scriptRoots);
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

        ImGui::TextUnformatted("Add Component");
        ImGui::Separator();

        const std::vector<ComponentClassEntry> entries = BuildComponentEntries();
        if (entries.empty())
        {
            DrawEmptyStateMessage("No components available.");
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

                const std::string& tooltip = !entry.info->QualifiedName.empty() ? entry.info->QualifiedName : entry.label;
                ShowTooltip(tooltip.c_str(), ImGuiHoveredFlags_DelayShort);
            }
        }

        ImGui::EndPopup();
    }

    void ComponentInspectorSection::Draw(Game::Actor& actor)
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

        DrawSeparatorText("Components");

        if (ImGui::Button("+ Add Component"))
        {
            ImGui::OpenPopup("AddComponentPopup");
        }

        ImGui::SameLine();
        DrawHelpMarker("Attach new behaviours to this actor.");
        DrawAddComponentPopup(actor);

        auto& components = actor.GetComponents();
        if (components.empty())
        {
            DrawEmptyStateMessage("Actor has no components.");
            return;
        }

        ScopedID componentsId("ActorComponents");
        ScopedColor headerColor(ImGuiCol_Header, ImVec4(0.25f, 0.33f, 0.45f, 0.65f));
        ScopedColor headerHoverColor(ImGuiCol_HeaderHovered, ImVec4(0.30f, 0.40f, 0.55f, 0.75f));
        ScopedColor headerActiveColor(ImGuiCol_HeaderActive, ImVec4(0.20f, 0.32f, 0.52f, 0.85f));
        ScopedStyle framePadding(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));

        static Game::Component* componentPendingRemoval = nullptr;
        for (std::size_t index = 0; index < components.size(); ++index)
        {
            auto& component = components[index];
            if (!component)
            {
                continue;
            }

            ScopedID componentId(static_cast<int>(index));

            std::string typeLabel;

            const Bix::Reflection::ClassInfo& classInfo = component->GetClass();
            if (!classInfo.Name.empty())
            {
                typeLabel = classInfo.Name;
            }
            else if (!classInfo.QualifiedName.empty())
            {
                typeLabel = classInfo.QualifiedName;
            }

            if (typeLabel.empty())
            {
                typeLabel = ToStdString(component->GetTypeName());
            }
            const std::string treeLabel = "🧩 " + typeLabel;

            const float startX = ImGui::GetCursorPosX();
            const float startY = ImGui::GetCursorPosY();

            bool open = ImGui::TreeNodeEx(component.get(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding, "%s", treeLabel.c_str());

            const float labelWidth = ImGui::CalcTextSize(treeLabel.c_str()).x;
            const float iconSpacing = ImGui::GetTreeNodeToLabelSpacing();

            const float buttonHeight = ImGui::GetFrameHeight();
            const float textHeight = ImGui::GetTextLineHeight();

            float centerOffsetY = (textHeight - buttonHeight) * 0.5f;
            float buttonPosX = startX + labelWidth + iconSpacing + ImGui::GetStyle().ItemInnerSpacing.x;
            float buttonPosY = startY + centerOffsetY;

            ImVec2 prevCursor = ImGui::GetCursorPos();
            ImGui::SetCursorPos(ImVec2(buttonPosX, buttonPosY));

            if (IconButton("🗑", "Remove this component"))
            {
                componentPendingRemoval = component.get();
            }

            ImGui::SetCursorPos(prevCursor);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
            ImGui::Dummy(ImVec2(0.0f, 0.0f));
            ImGui::PopStyleVar();

            if (open)
            {
                const float cursorBefore = ImGui::GetCursorPosY();
                const bool drewReflected = PropertyInspector::DrawClassProperties(component->GetClass(), component.get(), false, nullptr, false);
                const float cursorAfterReflected = ImGui::GetCursorPosY();

                component->DrawInspectorUI();
                const float cursorAfterCustom = ImGui::GetCursorPosY();

                if (!drewReflected && cursorAfterCustom <= cursorBefore + 0.5f && cursorAfterReflected <= cursorBefore + 0.5f)
                {
                    DrawEmptyStateMessage("No editable properties.");
                }

                ImGui::TreePop();
            }
        }

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
