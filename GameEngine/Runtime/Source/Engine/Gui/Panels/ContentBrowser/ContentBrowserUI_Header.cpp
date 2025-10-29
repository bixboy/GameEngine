#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserUI_Header.h"

#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Engine/Gui/Widgets/GuiWidgetLibrary.h"

#include "imgui.h"

namespace BixEngine::Gui
{
    namespace
    {
        constexpr ImVec4 kContentHeaderBackground{0.16f, 0.16f, 0.16f, 1.0f};
        constexpr float kContentHeaderHeight = 72.0f;
    }

    // ─────────────────────────────────────────────
    // 📦  Gestion du dossier courant et affichage
    // ─────────────────────────────────────────────

    void RenderHeader(ContentBrowserState& state, String& selectedEntry, char (&searchBuffer)[256])
    {
        namespace fs = std::filesystem;
        namespace Utils = Gui::Utils;
        using Widgets::PanelToolbar;

        const fs::path relativePath = state.current.lexically_relative(state.root);
        const String relativeString = relativePath.generic_string();
        const bool atRoot = relativeString.IsEmpty() || relativeString == ".";

        Gui::Utils::ScopedColor headerColor(ImGuiCol_ChildBg, kContentHeaderBackground);
        if (ImGui::BeginChild("ContentBrowserHeader", ImVec2(0.0f, kContentHeaderHeight), true, ImGuiWindowFlags_NoScrollbar))
        {
            Widgets::DrawPanelHeader({
                .title = "Content Browser",
                .subtitle = state.current.empty() ? "" : state.current.generic_string(),
                .showSeparator = false,
            });

            PanelToolbar toolbar{};
            toolbar.AddLeft([&]()
            {
                if (ImGui::Button("Content"))
                {
                    state.current = state.root;
                    selectedEntry.Clear();
                }
            });

            toolbar.AddLeft([&]()
            {
                ImGui::BeginDisabled(atRoot);
                if (ImGui::Button("Up"))
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
            });

            toolbar.AddLeft([&]()
            {
                const String display = atRoot ? String("Content") : relativeString;
                Utils::DrawDescriptionText(display.c_str());
            });

            toolbar.AddRight([&]()
            {
                Utils::InputTextWithLabel("Search", searchBuffer, IM_ARRAYSIZE(searchBuffer));
            });

            toolbar.Commit();
        }
        ImGui::EndChild();
    }
}

