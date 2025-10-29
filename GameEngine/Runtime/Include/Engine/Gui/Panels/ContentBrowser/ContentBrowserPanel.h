#pragma once
#include "Engine/Gui/Core/GuiManager.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Gui/DefaultEngineGui.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserPanelInternal.h"


namespace BixEngine::Gui
{

    class ContentBrowserPanel
    {
    public:
        explicit ContentBrowserPanel(const DefaultEngineGuiContext& context);

        void Draw();

    private:
        
        void EnsureValidDirectory();

        void HandleShortcuts();

        ContentBrowserState state_;

        char searchBuffer_[256] = "";
        String selectedEntry_;
        PopupRequestState popupRequests_;
    };
    
    GuiPanel& CreateContentBrowserPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}
