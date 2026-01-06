#include "Gui/Panels/ActorInspector/InspectorSections/GeneralInspectorSection.h"
#include "Gui/Panels/ActorInspector/ActorInspectorState.h"
#include "Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Framework/Actor.h"
#include <imgui.h>
#include <string>
#include "Gui/Panels/ActorInspector/PropertyInspector.h"
#include "Gui/Widgets/Controls/DrawBadge.h"
#include "Gui/Widgets/Layout/SectionContainer.h"


namespace BixEngine::Gui::ActorInspector
{
    using namespace Theme;
    using namespace GuiUtils;
    using namespace BixEngine::Gui::Widgets;

    namespace
    {
        constexpr ImVec4 kColorActiveBg   = ImVec4(0.27f, 0.58f, 0.32f, 1.0f); // Vert
        constexpr ImVec4 kColorInactiveBg = ImVec4(0.45f, 0.45f, 0.45f, 1.0f); // Gris sombre
        constexpr ImVec4 kColorTextLight  = ImVec4(0.95f, 0.98f, 0.96f, 1.0f); // Blanc cassé
        constexpr ImVec4 kColorInfoBg     = ImVec4(0.28f, 0.44f, 0.72f, 1.0f); // Bleu

        void DrawActorOverview(Game::Actor& actor, ActorInspectorState& state)
        {
            GuiUtils::ScopedID overviewId("ActorOverview");
            
            GuiUtils::ScopedColor background(ImGuiCol_ChildBg, OverviewBackground);
            GuiUtils::ScopedStyle rounding(ImGuiStyleVar_ChildRounding, 6.0f); // Arrondi plus doux
            GuiUtils::ScopedStyle padding(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));

            const bool visible = ImGui::BeginChild("OverviewCard", ImVec2(0.0f, 0.0f), 
                ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border, 
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysUseWindowPadding);

            if (visible)
            {
                GuiUtils::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));

                if (ImGui::BeginTable("ActorOverviewTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_NoSavedSettings))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("Name");
                    
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    
                    if (ImGui::InputText("##ActorName", state.nameBuffer.data(), state.nameBuffer.size(), ImGuiInputTextFlags_AutoSelectAll))
                    {
                        actor.SetName(state.nameBuffer.data());
                    }

                    bool isActive = actor.IsActive();
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("Status");
                    
                    ImGui::TableSetColumnIndex(1);
                    ImGui::BeginGroup();
                    {
                        if (ImGui::Checkbox("##ActorActive", &isActive))
                        {
                            actor.SetActive(isActive);
                        }
                        
                        ImGui::SameLine(0.0f, 8.0f);
                        
                        const char* statusLabel = isActive ? "Active" : "Inactive";
                        const ImVec4& statusBg = isActive ? kColorActiveBg : kColorInactiveBg;
                        DrawBadge(statusLabel, statusBg, kColorTextLight);
                    }
                    
                    ImGui::EndGroup();
                    
                    const auto typeNameView = actor.GetTypeName().View();
                    
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("Type");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.*s", static_cast<int>(typeNameView.size()), typeNameView.data());

                    const auto uuidView = actor.GetUUID().View();
                    
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("ID");
                    ImGui::TableSetColumnIndex(1);
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                    ImGui::Text("%.*s", static_cast<int>(uuidView.size()), uuidView.data());
                    ImGui::PopStyleColor();
                    
                    if (ImGui::BeginPopupContextItem("CopyUUID"))
                    {
                        if (ImGui::MenuItem("Copy UUID"))
                            ImGui::SetClipboardText(std::string(uuidView).c_str());
                        
                        ImGui::EndPopup();
                    }

                    const auto& components = actor.GetComponents();
                    std::string componentLabel = std::to_string(components.size());
                    componentLabel += (components.size() == 1) ? " component" : " components";
                    
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("Composition");
                    ImGui::TableSetColumnIndex(1);
                    DrawBadge(componentLabel.c_str(), kColorInfoBg, kColorTextLight);

                    ImGui::EndTable();
                }
            }
            ImGui::EndChild();
        }
    }

    void GeneralInspectorSection::Draw(Game::Actor& actor)
    {
        const std::string contextId = BuildActorContextId(actor);
        PersistentSectionScope section("General", contextId);
        
        if (!section.IsOpen())
        {
            return;
        }

        Layout::SectionContainer container("GeneralSection");
        if (!container.IsVisible())
        {
            return;
        }

        ActorInspectorState& state = GetActorState(actor);
        
        DrawActorOverview(actor, state);
        
        ImGui::Spacing();
        PropertyInspector::DrawClassProperties(actor.GetClass(), &actor, true, "Properties", false);
    }
}
