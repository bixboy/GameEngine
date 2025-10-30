#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Engine/Gui/Internal/GuiDocking.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Core/Logger.h"
#include <filesystem>
#include "imgui.h"


namespace fs = std::filesystem;

namespace BixEngine::Gui
{
    using namespace Theme;
    using namespace Utils;

    // ─────────────────────────────────────────────
    // 🏗️  Implémentation du Content Browser unique
    // ─────────────────────────────────────────────

    ContentBrowserPanel::ContentBrowserPanel(const DefaultEngineGuiContext& context)
    {
        state_.openScriptFilesCallback = context.openScriptFilesInEditor;
        state_.openAssetEditorCallback = context.openAssetInEditor;
    }

    void ContentBrowserPanel::EnsureValidDirectory()
    {
        if (state_.current.empty() || !fs::exists(state_.current))
            state_.current = state_.root;
    }

    void ContentBrowserPanel::HandleShortcuts()
    {
        if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            return;

        // 🔁 Refresh du dossier
        if (ImGui::IsKeyPressed(ImGuiKey_F5))
        {
            LOG_INFO("Content Browser refreshed");
            state_.error.Clear();
            EnsureContentBrowserInitialized(state_);
        }

        // 🗑️ Suppression
        if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        {
            if (!selectedEntry_.IsEmpty())
            {
                LOG_WARNING("Delete requested for: " + selectedEntry_);
            }
        }
    }

    void ContentBrowserPanel::Draw()
    {
        if (!EnsureContentBrowserInitialized(state_))
        {
            Utils::DrawErrorMessage(state_.error);
            ImGui::Spacing();
            Utils::DrawEmptyStateMessage("The Content Browser requires access to the Content directory.");
            return;
        }

        EnsureValidDirectory();

        // ─────────────────────────────
        // Barre de navigation du dossier
        // ─────────────────────────────
        RenderHeader(state_, selectedEntry_, searchBuffer_);
        ImGui::Spacing();

        // ─────────────────────────────
        // Arborescence + fichiers
        // ─────────────────────────────
        ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 6.f));

        RenderDirectoryTree(state_, selectedEntry_);
        ImGui::SameLine();

        const String searchQuery(searchBuffer_);
        RenderEntries(state_, selectedEntry_, popupRequests_, searchQuery);

        // Popups (création / renommage / erreurs)
        RenderPopups(state_, selectedEntry_, popupRequests_);

        HandleShortcuts();
    }

    // ─────────────────────────────────────────────
    // 🧱  Création unique du panneau dockable
    // ─────────────────────────────────────────────

    GuiPanel& CreateContentBrowserPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        GuiPanel& contentPanel = guiManager.CreatePanel("content_browser", "Content Browser");
        guiManager.SetPanelDockingArea(contentPanel, DockSpaceRegion::Bottom);
        contentPanel.SetResizable(true);
        contentPanel.SetMovable(true);
        contentPanel.SetCollapsable(true);
        contentPanel.SetClosable(true);
        contentPanel.SetBackgroundColor(ContentBackground);
        contentPanel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);

        static ContentBrowserPanel browser(context);
        contentPanel.SetDrawFunction([] { browser.Draw(); });

        return contentPanel;
    }
}
