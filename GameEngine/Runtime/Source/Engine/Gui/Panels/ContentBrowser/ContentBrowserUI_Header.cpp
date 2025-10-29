#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserUI_Header.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Engine/Gui/Widgets/GuiWidgetLibrary.h"
#include "imgui.h"

namespace BixEngine::Gui
{
    namespace
    {
        constexpr ImVec4 kHeaderBackground{0.16f, 0.16f, 0.16f, 1.0f};
        constexpr ImVec4 kBreadcrumbHighlight{0.95f, 0.80f, 0.40f, 1.0f};
        constexpr ImVec4 kBreadcrumbNormal{0.75f, 0.75f, 0.75f, 1.0f};
        constexpr float kHeaderHeight = 72.0f;
        constexpr float kButtonHeight = 24.0f;
        constexpr float kButtonWidth = 76.0f;
    }

    void RenderHeader(ContentBrowserState& state, String& selectedEntry, char (&searchBuffer)[256])
    {
        namespace fs = std::filesystem;

        Utils::ScopedColor headerColor(ImGuiCol_ChildBg, kHeaderBackground);
        if (!ImGui::BeginChild("ContentBrowserHeader", ImVec2(0.0f, kHeaderHeight), true, ImGuiWindowFlags_NoScrollbar))
        {
            ImGui::EndChild();
            return;
        }

        ImGui::PushID("HeaderScope");

        Widgets::DrawPanelHeader({
            .title = "Content Browser",
            .subtitle = "",
            .showSeparator = false,
        });

        Widgets::PanelToolbar toolbar{};

        // ---------------------------------------------------
        // BOUTONS NAVIGATION
        // ---------------------------------------------------
        toolbar.AddLeft([&]()
        {
            { // <- ouverture d’un scope explicite RAII
                Utils::ScopedStyle rounding(ImGuiStyleVar_FrameRounding, 5.0f);
                Utils::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 0.0f));

                const ImVec2 btnSize(kButtonWidth, kButtonHeight);

                if (ImGui::Button("Content##Nav", btnSize))
                {
                    state.current = state.root;
                    selectedEntry.Clear();
                }

                ImGui::SameLine();
                ImGui::BeginDisabled(state.current == state.root);
                if (ImGui::Button("Up##Nav", btnSize))
                {
                    fs::path parent = state.current.parent_path();
                    fs::path parentRelative = parent.lexically_relative(state.root);
                    const String parentString = parentRelative.generic_string();

                    if (parentString.IsEmpty() || parentString == "." || parentString.View().rfind("..", 0) == 0)
                        state.current = state.root;
                    else
                        state.current = parent;

                    selectedEntry.Clear();
                }
                ImGui::EndDisabled();
            } // <- destruction assurée ici
        });

        // ---------------------------------------------------
        // BREADCRUMB
        // ---------------------------------------------------
        toolbar.AddLeft([&]()
        {
            { // Scope explicite
                Utils::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(3.0f, 0.0f));
                Utils::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));

                fs::path currentPath = state.root;
                std::vector<fs::path> segments;
                for (const auto& part : state.current.lexically_relative(state.root))
                    segments.push_back(part);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.30f, 0.30f, 0.4f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.35f, 0.5f));

                if (ImGui::Button("Content##Breadcrumb"))
                {
                    state.current = state.root;
                    selectedEntry.Clear();
                }

                for (size_t i = 0; i < segments.size(); ++i)
                {
                    ImGui::SameLine();
                    ImGui::TextUnformatted("›");
                    ImGui::SameLine();

                    currentPath /= segments[i];
                    const std::string segmentName = segments[i].generic_string();

                    const bool isLast = (i == segments.size() - 1);
                    const ImVec4 color = isLast ? kBreadcrumbHighlight : kBreadcrumbNormal;

                    Utils::ScopedColor textColor(ImGuiCol_Text, color);
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::Button(segmentName.c_str()))
                    {
                        state.current = currentPath;
                        selectedEntry.Clear();
                    }
                    ImGui::PopID();
                }

                ImGui::PopStyleColor(3);
            }
        });

        // ---------------------------------------------------
        // RECHERCHE
        // ---------------------------------------------------
        toolbar.AddRight([&]()
        {
            { // Scope explicite
                Utils::ScopedStyle rounding(ImGuiStyleVar_FrameRounding, 4.0f);
                Utils::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));
                ImGui::SetNextItemWidth(200.0f);
                Utils::SearchInput("##Search", searchBuffer, IM_ARRAYSIZE(searchBuffer), "Search assets...");
            }
        });

        toolbar.Commit();

        ImGui::PopID();
        ImGui::EndChild();
    }
}
