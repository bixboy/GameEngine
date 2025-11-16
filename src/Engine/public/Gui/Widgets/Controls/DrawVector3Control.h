#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    /**
     * \brief Contrôle d'édition d'un vecteur3 composé de trois DragFloat avec boutons de remise à zéro.
     * \param label Libellé affiché dans la première colonne.
     * \param values Tableau de trois valeurs modifiables (X, Y, Z).
     * \param resetValue Valeur appliquée lorsqu'un bouton d'axe est cliqué.
     * \param speed Vitesse de glissement utilisée par ImGui::DragFloat.
     * \param format Format d'affichage ("%.3f" par défaut si nullptr).
     * \return true si au moins une composante a été modifiée.
     */
    bool DrawVector3Control(const char* label, float* values, float resetValue, float speed, const char* format);
}
