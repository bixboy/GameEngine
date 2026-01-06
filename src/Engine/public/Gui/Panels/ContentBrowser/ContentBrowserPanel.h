#pragma once
#include "Gui/Panels/GuiPanelBase.h"
#include "Gui/Core/DefaultEngineGui.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include <filesystem>
#include <vector>
#include <chrono>


namespace BixEngine::Gui
{
    class ContentBrowserPanel : public GuiPanelBase
    {
    public:
        explicit ContentBrowserPanel(const DefaultEngineGuiContext& context);
        ~ContentBrowserPanel() override;

        void Draw() override;
        void HandleShortcuts() override;
        void OnOpen() override;
        void OnClose() override;

        void ImportExternalFiles(const std::vector<std::filesystem::path>& paths);
        static ContentBrowserPanel* GetActiveInstance() noexcept;

    private:
        // --- Méthodes UI Internes ---
        void DrawHeader();
        void DrawBody();
        void DrawDirectoryTree();
        void DrawEntries(const String& searchQuery);
        void DrawPopups();

        // --- Logique Métier ---
        void EnsureValidDirectory();
        bool RefreshDirectoryCache();
        bool DeleteScriptFiles(const ContentEntry& entry, String& error);

    private:
        // --- Données ---
        static ContentBrowserPanel* activeInstance_;
        ContentBrowserState state_;

        // UI State
        char searchBuffer_[256] = "";
        String selectedEntry_; 
        PopupRequestState popupRequests_;

        // --- Gestion du Refresh & Timers ---
        bool m_PendingRefresh = false;
        float m_RefreshTimer = 0.0f;
        std::chrono::steady_clock::time_point m_LastImportTime;
    };
}