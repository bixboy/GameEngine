#include "Bix/Engine/Gui/GuiManager.h"
#include "Bix/Engine/Gui/GuiPanel.h"
#include "imgui.h"
#include <filesystem>
#include "Bix/Engine/Gui/DefaultEngineGui.h"
#include "Bix/Engine/Gui/Panels/ContentBrowser/ContentBrowserPanelInternal.h"


namespace BixEngine::Gui
{
    GuiPanel& CreateContentBrowserPanel(GuiManager& guiManager, const DefaultEngineGuiContext&)
    {
        GuiPanel& contentPanel = guiManager.CreatePanel("content_browser", "Content Browser");
        guiManager.SetPanelDockingArea(contentPanel, DockSpaceRegion::Bottom);
        contentPanel.SetResizable(true);
        contentPanel.SetMovable(true);
        contentPanel.SetCollapsable(true);
        contentPanel.SetClosable(true);
        contentPanel.SetBackgroundColor(kContentBackground);
        contentPanel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);
        const auto scriptEditorOpener = context.openScriptFilesInEditor;
        contentPanel.SetDrawFunction([scriptEditorOpener]()
        {
            namespace fs = std::filesystem;

            static ContentBrowserState state{};
            static char searchBuffer[256] = "";
            static String selectedEntry{};
            static PopupRequestState popupRequests{};

            state.openScriptFilesCallback = scriptEditorOpener;

            if (!EnsureContentBrowserInitialized(state))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
                ImGui::TextWrapped("%s", state.error.c_str());
                ImGui::PopStyleColor();
                ImGui::Spacing();
                ImGui::TextDisabled("The Content Browser requires access to the Content directory.");
                return;
            }

            if (state.current.empty())
                state.current = state.root;

            if (!fs::exists(state.current))
                state.current = state.root;

            RenderHeader(state, selectedEntry, searchBuffer);

            ImGui::BeginGroup();
            RenderDirectoryTree(state, selectedEntry);
            ImGui::SameLine();

            const String searchQuery(searchBuffer);
            RenderEntries(state, selectedEntry, popupRequests, searchQuery);
            ImGui::EndGroup();

            RenderPopups(state, selectedEntry, popupRequests);
        });

        return contentPanel;
    }
}
