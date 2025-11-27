#include "Gui/Panels/ActorInspector/InspectorSections/GeneralInspectorSection.h"
#include "Gui/Panels/ActorInspector/ActorInspectorState.h"
#include "Gui/Widgets/Widgets.h"
#include "Gui/Panels/ActorInspector/Utils/ActorInspectorHelpers.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Framework/Actor.h"
#include <imgui.h>
#include <string>

#include "Gui/Panels/ActorInspector/PropertyInspector.h"


namespace BixEngine::Gui::ActorInspector
{
    using namespace Theme;
    using namespace Utils;
    using namespace BixEngine::Gui::Widgets;

    namespace
    {
        void DrawActorOverview(Game::Actor& actor, ActorInspectorState& state)
        {
            ScopedID overviewId("ActorOverview");
            ScopedColor background(ImGuiCol_ChildBg, OverviewBackground);
            ScopedStyle rounding(ImGuiStyleVar_ChildRounding, 10.0f);
            ScopedStyle padding(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));

            const bool visible = ImGui::BeginChild("OverviewCard", ImVec2(-FLT_MIN, 0.0f),
                ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysUseWindowPadding);

            if (visible)
            {
                ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));

                if (ImGui::BeginTable("ActorOverviewTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_NoSavedSettings))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("Name");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-1.0f);
                    if (ImGui::InputText("##ActorName", state.nameBuffer.data(), state.nameBuffer.size()))
                    {
                        actor.SetName(state.nameBuffer.data());
                    }

                    bool isActive = actor.IsActive();
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("Status");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::BeginGroup();
                    
                    if (ImGui::Checkbox("##ActorActive", &isActive))
                    {
                        actor.SetActive(isActive);
                    }
                    
                    ImGui::SameLine(0.0f, 6.0f);
                    
                    const ImVec4 statusColor = isActive ? ImVec4(0.27f, 0.58f, 0.32f, 1.0f) : ImVec4(0.45f, 0.45f, 0.45f, 1.0f);
                    constexpr auto statusTextColor = ImVec4(0.95f, 0.98f, 0.96f, 1.0f);
                    DrawBadge(isActive ? "Active" : "Inactive", statusColor, statusTextColor);
                    
                    ImGui::EndGroup();

                    const std::string typeName = ToStdString(actor.GetTypeName());
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("Type");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(typeName.c_str());

                    const std::string uuid = ToStdString(actor.GetUUID());
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("ID");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(uuid.c_str());

                    const auto& components = actor.GetComponents();
                    std::string componentLabel = std::to_string(components.size());
                    componentLabel += components.size() == 1 ? " component" : " components";
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("Composition");
                    ImGui::TableSetColumnIndex(1);
                    DrawBadge(componentLabel.c_str(), ImVec4(0.28f, 0.44f, 0.72f, 1.0f), ImVec4(0.95f, 0.97f, 1.0f, 1.0f));

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

        SectionContainer container("GeneralSection");
        if (!container.IsVisible())
        {
            return;
        }

        ActorInspectorState& state = GetActorState(actor);
        DrawActorOverview(actor, state);
        PropertyInspector::DrawClassProperties(actor.GetClass(), &actor, true, "Properties", false);
    }
}
