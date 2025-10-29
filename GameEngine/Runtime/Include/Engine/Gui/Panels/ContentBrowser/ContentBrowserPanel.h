#pragma once
#include "Engine/Gui/Core/GuiManager.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Gui/DefaultEngineGui.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserPanelInternal.h"


namespace BixEngine::Gui
{
    /**
     *  Classe responsable du rendu et de la gestion du panneau Content Browser.
     */
    class ContentBrowserPanel
    {
    public:
        explicit ContentBrowserPanel(const DefaultEngineGuiContext& context);

        /** Dessine le contenu du panneau à chaque frame */
        void Draw();

    private:
        /** Assure que le répertoire courant est valide */
        void EnsureValidDirectory();

        /** Gestion des raccourcis clavier */
        void HandleShortcuts();

        /** État principal du content browser */
        ContentBrowserState state_;

        /** Buffers temporaires UI */
        char searchBuffer_[256] = "";
        String selectedEntry_;
        PopupRequestState popupRequests_;
    };

    /**
     *  Fabrique un panneau dockable standard pour le Content Browser.
     *  Configure la fenêtre et connecte le rendu au ContentBrowserPanel.
     */
    GuiPanel& CreateContentBrowserPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}
