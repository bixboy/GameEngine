#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Engine/Gui/GuiDocking.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"
#include "Core/Logger.h"
#include <filesystem>
#include <system_error>
#include "imgui.h"
#include <memory>
#include <stdexcept>

namespace fs = std::filesystem;

namespace BixEngine::Gui
{
    static bool g_PendingRefreshAfterImport = false;

    ContentBrowserPanel* ContentBrowserPanel::activeInstance_ = nullptr;

    using namespace Theme;
    using namespace Utils;

    // ─────────────────────────────────────────────
    // Implémentation du Content Browser unique
    // ─────────────────────────────────────────────

    ContentBrowserPanel::ContentBrowserPanel(const DefaultEngineGuiContext& context) : GuiPanelBase("Content Browser")
    {
        state_.openScriptFilesCallback = context.openScriptFilesInEditor;
        state_.openAssetEditorCallback = context.openAssetInEditor;
        activeInstance_ = this;
    }

    ContentBrowserPanel::~ContentBrowserPanel()
    {
        if (activeInstance_ == this)
            activeInstance_ = nullptr;
    }

    ContentBrowserPanel* ContentBrowserPanel::GetActiveInstance() noexcept
    {
        return activeInstance_;
    }

    void ContentBrowserPanel::ImportExternalFiles(const std::vector<std::filesystem::path>& paths)
    {
        if (paths.empty())
            return;

        if (!EnsureContentBrowserInitialized(state_))
            return;

        EnsureValidDirectory();

        const fs::path targetDirectory = (!state_.current.empty() && fs::exists(state_.current) && fs::is_directory(state_.current))
            ? state_.current
            : state_.root;

        if (targetDirectory.empty() || !fs::exists(targetDirectory))
            return;

        bool copiedAny = false;

        for (const auto& source : paths)
        {
            if (source.empty())
                continue;

            std::error_code statusError;
            if (!fs::exists(source, statusError) || statusError)
                continue;

            if (!fs::is_regular_file(source, statusError) || statusError)
                continue;

            fs::path destination = targetDirectory / source.filename();

            std::error_code equivalentError;
            if (fs::exists(destination) && fs::equivalent(source, destination, equivalentError) && !equivalentError)
                continue;

            fs::path finalDestination = destination;
            int suffix = 1;
            while (fs::exists(finalDestination))
            {
                const std::string baseName = destination.stem().string();
                const std::string extension = destination.extension().string();
                finalDestination = destination.parent_path() / (baseName + "_" + std::to_string(suffix++) + extension);
            }

            String copyError;
            if (!TryCopyFile(source, finalDestination, copyError))
            {
                state_.error = copyError;

                String message = "Failed to import file: ";
                message += String(source.generic_string().c_str());
                message += " -> ";
                message += copyError;

                LOG_ERROR(message);
                continue;
            }

            copiedAny = true;
            state_.error.Clear();

            selectedEntry_ = String(finalDestination.generic_string().c_str());

            String successMessage = "Imported file into Content Browser: ";
            successMessage += String(finalDestination.generic_string().c_str());
            LOG_INFO(successMessage);
        }
        
        if (copiedAny)
        {
            g_PendingRefreshAfterImport = true;
        }
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

        if (ImGui::IsKeyPressed(ImGuiKey_F5))
        {
            LOG_INFO("Content Browser refreshed");
            state_.error.Clear();
            EnsureContentBrowserInitialized(state_);
            state_.cache.dirty = true;
            state_.cache.ClearMeta();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Delete))
        {
            if (!selectedEntry_.IsEmpty())
                LOG_WARNING("Delete requested for: " + selectedEntry_);
        }
    }

    void ContentBrowserPanel::Draw()
    {
        if (!EnsureContentBrowserInitialized(state_))
        {
            DrawErrorMessage(state_.error);
            ImGui::Spacing();
            DrawEmptyStateMessage("The Content Browser requires access to the Content directory.");
            return;
        }

        EnsureValidDirectory();

        DrawHeader();
        ImGui::Spacing();
        DrawBody();

        RenderPopups(state_, selectedEntry_, popupRequests_);

        if (g_PendingRefreshAfterImport)
        {
            g_PendingRefreshAfterImport = false;
            state_.cache.dirty = true;
            state_.cache.ClearMeta();
            LOG_INFO("Content Browser cache refreshed after import");
        }
    }

    void ContentBrowserPanel::DrawHeader()
    {
        RenderHeader(state_, selectedEntry_, searchBuffer_);
    }

    void ContentBrowserPanel::DrawBody()
    {
        ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 6.f));
        RenderDirectoryTree(state_, selectedEntry_);
        ImGui::SameLine();

        const String searchQuery(searchBuffer_);
        RenderEntries(state_, selectedEntry_, popupRequests_, searchQuery);
    }

    void ContentBrowserPanel::OnOpen()
    {
        activeInstance_ = this;
    }

    void ContentBrowserPanel::OnClose()
    {
    }

    // ─────────────────────────────────────────────
    // Création unique du panneau dockable
    // ─────────────────────────────────────────────

    GuiPanel& CreateContentBrowserPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        GuiManager::RegisterPanel<ContentBrowserPanel>("Content Browser", context);

        GuiPanelBase* controller = guiManager.CreatePanelByName("Content Browser");
        if (!controller)
            throw std::runtime_error("Failed to create Content Browser panel from registry.");

        GuiPanel* panel = controller->GetPanel();
        if (!panel)
            throw std::runtime_error("Content Browser controller has no associated GuiPanel.");

        guiManager.SetPanelDockingArea(*panel, DockSpaceRegion::Bottom);
        panel->SetResizable(true);
        panel->SetMovable(true);
        panel->SetCollapsable(true);
        panel->SetClosable(true);
        panel->SetBackgroundColor(ContentBackground);
        panel->AddWindowFlags(ImGuiWindowFlags_NoCollapse);

        return *panel;
    }
}
