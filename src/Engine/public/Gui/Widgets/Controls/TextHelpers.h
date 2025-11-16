#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets::Controls::TextHelpers
{
    /**
     * \brief Dessine une étiquette dans la première colonne d'un contrôle propriété.
     * \param label Texte de l'étiquette (optionnel).
     * \param columnWidth Largeur réservée à la première colonne.
     */
    void DrawPropertyLabel(const char* label, float columnWidth) noexcept;

    /**
     * \brief Affiche une info-bulle d'aide standardisée.
     */
    void DrawHelpTooltip(const char* text) noexcept;

    /**
     * \brief Calcule la taille d'un badge texte avec padding interne.
     */
    ImVec2 ComputeBadgeSize(const char* text, const ImVec2& padding) noexcept;
}
