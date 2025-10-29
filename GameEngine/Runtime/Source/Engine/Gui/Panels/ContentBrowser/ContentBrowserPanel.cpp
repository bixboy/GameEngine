#include "Engine/Gui/Core/GuiManager.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "imgui.h"
#include <filesystem>
#include "Engine/Gui/DefaultEngineGui.h"
#include "Engine/Gui/Core/GuiDocking.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserPanelInternal.h"


namespace BixEngine::Gui
{
    GuiPanel& CreateContentBrowserPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
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
        const auto actorEditorOpener = context.openActorInEditor;
        contentPanel.SetDrawFunction([scriptEditorOpener, actorEditorOpener]()
        {
            namespace fs = std::filesystem;

            static ContentBrowserState state{};
            static char searchBuffer[256] = "";
            static String selectedEntry{};
            static PopupRequestState popupRequests{};

            state.openScriptFilesCallback = scriptEditorOpener;
            state.openActorEditorCallback = actorEditorOpener;

            if (!EnsureContentBrowserInitialized(state))
            {
                Utils::DrawErrorMessage(state.error);
                ImGui::Spacing();
                Utils::DrawEmptyStateMessage("The Content Browser requires access to the Content directory.");
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
