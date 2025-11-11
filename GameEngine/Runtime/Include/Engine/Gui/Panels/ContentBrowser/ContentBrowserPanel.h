#pragma once
#include "Engine/Gui/Internal/GuiManager.h"
#include "Engine/Gui/Internal/GuiPanel.h"
#include "Engine/Gui/DefaultEngineGui.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserPanelInternal.h"

#include <filesystem>
#include <vector>


namespace BixEngine::Gui
{

    class ContentBrowserPanel
    {
    public:
        explicit ContentBrowserPanel(const DefaultEngineGuiContext& context);

        void Draw();

        void ImportExternalFiles(const std::vector<std::filesystem::path>& paths);

        static ContentBrowserPanel* GetActiveInstance() noexcept;

    private:

        void EnsureValidDirectory();

        void HandleShortcuts();

        ContentBrowserState state_;

        char searchBuffer_[256] = "";
        String selectedEntry_;
        PopupRequestState popupRequests_;

        static ContentBrowserPanel* activeInstance_;
    };

    GuiPanel& CreateContentBrowserPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}
