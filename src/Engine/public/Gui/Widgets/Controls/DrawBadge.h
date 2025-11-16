#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    /**
     * \brief Dessine un badge rectangulaire coloré (texte centré + padding).
     * \param label Texte à afficher dans le badge.
     * \param backgroundColor Couleur de fond du badge.
     * \param textColor Couleur du texte.
     * \return Taille finale du badge en pixels (utile pour des alignements personnalisés).
     */
    ImVec2 DrawBadge(const char* label, const ImVec4& backgroundColor, const ImVec4& textColor);
}
