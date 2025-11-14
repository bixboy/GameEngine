#pragma once
#include <string>

#include "Gui/Widgets/Layout/PanelToolbar.h"

namespace BixEngine::Gui::Widgets
{
    /**
     * \brief Options de rendu pour l'en-tête d'un panneau ImGui custom.
     */
    struct PanelHeaderOptions
    {
        std::string title{};
        std::string subtitle{};
        bool showSeparator{true};
    };

    /**
     * \brief Dessine un en-tête simple composé d'un titre et d'un sous-titre optionnel.
     */
    void DrawPanelHeader(const PanelHeaderOptions& options);

    /**
     * \brief Builder simplifiant la composition d'un panneau (header + toolbar).
     */
    class PanelBuilder
    {
    public:
        explicit PanelBuilder(PanelHeaderOptions options);

        /** Dessine l'en-tête configuré. */
        void DrawHeader() const;

        /** Retourne la toolbar associée afin d'enregistrer les callbacks UI. */
        PanelToolbar& Toolbar() noexcept { return toolbar_; }

        /** Finalise la barre d'outils (appel implicite à PanelToolbar::Commit). */
        void DrawToolbar();

    private:
        PanelHeaderOptions options_{};
        PanelToolbar toolbar_{};
    };
}
