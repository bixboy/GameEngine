#include "Gui/Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Utils/ContentBrowserUtils.h"
#include "Utils/FileIO/FilesUtils.h" // Restauré
#include "Debug/Logger.h"

#include <filesystem>
#include <imgui.h>
#include <chrono>

namespace fs = std::filesystem;

namespace BixEngine::Gui
{
    ContentBrowserPanel* ContentBrowserPanel::activeInstance_ = nullptr;
    
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
        static std::chrono::steady_clock::time_point lastImportTime;
        auto now = std::chrono::steady_clock::now();
        
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastImportTime).count() < 500)
            return;
        
        lastImportTime = now;

        if (paths.empty())
            return;

        if (!ContentBrowserUtils::EnsureContentBrowserInitialized(state_))
            return;
        
        EnsureValidDirectory();

        const fs::path targetDirectory = (!state_.current.empty() && fs::exists(state_.current)) ? state_.current : state_.root;
        bool copiedAny = false;

        // 2. Logique d'import propre via FilesUtils
        for (const auto& source : paths)
        {
            if (source.empty()) continue;

            fs::path destination = targetDirectory / source.filename();
            fs::path finalDestination = destination;
            
            int suffix = 1;
            while (fs::exists(finalDestination))
            {
                finalDestination = destination.parent_path() / (destination.stem().string() + "_" + std::to_string(suffix++) + destination.extension().string());
            }

            String copyError;
            if (FilesUtils::Utilities::TryCopyFile(source, finalDestination, true, copyError))
            {
                copiedAny = true;
                LOG_INFO("Import success: " + finalDestination.generic_string());
            }
            else
            {
                LOG_ERROR("Import failed for " + source.generic_string() + ": " + copyError);
            }
        }

        if (copiedAny)
        {
            LOG_INFO("Files copied. Scheduling refresh...");
            
            m_PendingRefresh = true;
            m_RefreshCountdown = 10; 
            selectedEntry_.Clear(); 
        }
    }

    void ContentBrowserPanel::EnsureValidDirectory()
    {
        std::error_code ec;
        if (state_.current.empty() || !fs::exists(state_.current, ec))
            state_.current = state_.root;
    }

    void ContentBrowserPanel::HandleShortcuts()
    {
        if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            return;

        if (ImGui::IsKeyPressed(ImGuiKey_F5))
        {
            state_.cache.dirty = true;
            state_.cache.ClearMeta();
        }
    }

    void ContentBrowserPanel::Draw()
    {
        // ─────────────────────────────────────────────────────────
        // REFRESH DÉCALÉ (Safe Refresh)
        // ─────────────────────────────────────────────────────────
        if (m_PendingRefresh)
        {
            if (m_RefreshCountdown > 0)
            {
                m_RefreshCountdown--;
            }
            else
            {
                state_.cache.dirty = true;
                state_.cache.ClearMeta();
                selectedEntry_.Clear(); 
                
                m_PendingRefresh = false;
            }
        }

        if (!ContentBrowserUtils::EnsureContentBrowserInitialized(state_))
        {
            Utils::DrawEmptyStateMessage("Content folder invalid.");
            return;
        }

        EnsureValidDirectory();

        DrawHeader();
        ImGui::Spacing();
        DrawBody();

        RenderPopups(state_, selectedEntry_, popupRequests_);
    }

    void ContentBrowserPanel::DrawHeader()
    {
        RenderHeader(state_, selectedEntry_, searchBuffer_);
    }

    void ContentBrowserPanel::DrawBody()
    {
        Utils::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 6.f));
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
}