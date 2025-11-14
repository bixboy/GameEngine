#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets::ThemeColors
{
    /** Couleur verte utilisée pour signaler un état performant ou stable. */
    inline constexpr ImVec4 PerformanceGood{0.2f, 1.0f, 0.2f, 1.0f};

    /** Couleur jaune utilisée pour signaler un avertissement léger. */
    inline constexpr ImVec4 PerformanceWarning{1.0f, 1.0f, 0.2f, 1.0f};

    /** Couleur rouge utilisée pour signaler un état critique. */
    inline constexpr ImVec4 PerformanceCritical{1.0f, 0.2f, 0.2f, 1.0f};

    /** Couleur neutre dédiée aux badges d'information. */
    inline constexpr ImVec4 BadgeBackground{0.20f, 0.24f, 0.32f, 1.0f};

    /** Couleur de texte standard pour les badges d'information. */
    inline constexpr ImVec4 BadgeText{0.92f, 0.95f, 1.0f, 1.0f};
}
