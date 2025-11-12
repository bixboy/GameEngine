#pragma once
#include "Gui/GuiManager.h"
#include "Gui/GuiPanelBase.h"
#include "Gui/DefaultEngineGui.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserPanelInternal.h"

#include <filesystem>
#include <vector>


namespace BixEngine::Gui
{
    class ContentBrowserPanel : public GuiPanelBase
    {
    public:
        explicit ContentBrowserPanel(const DefaultEngineGuiContext& context);
        ~ContentBrowserPanel() override;

        void Draw() override;

        void DrawHeader() override;
        void DrawBody() override;
        void HandleShortcuts() override;
        void OnOpen() override;
        void OnClose() override;

        void ImportExternalFiles(const std::vector<std::filesystem::path>& paths);

        static ContentBrowserPanel* GetActiveInstance() noexcept;

    private:
        void EnsureValidDirectory();

        ContentBrowserState state_;

        char searchBuffer_[256] = "";
        String selectedEntry_;
        PopupRequestState popupRequests_;

        static ContentBrowserPanel* activeInstance_;
    };

    GuiPanel& CreateContentBrowserPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}
