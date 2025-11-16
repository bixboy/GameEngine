#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets::Controls::ColorHelpers
{
    /**
     * \brief Calcule une couleur de survol adaptée à un bouton à partir d'une couleur de base.
     */
    ImVec4 ComputeHoveredColor(const ImVec4& baseColor) noexcept;

    /**
     * \brief Calcule une couleur active (clic) légèrement assombrie par rapport à la couleur de base.
     */
    ImVec4 ComputeActiveColor(const ImVec4& baseColor) noexcept;
}
