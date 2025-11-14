#pragma once
#include "Gui/Widgets/Styling/ThemeColors.h"

namespace BixEngine::Gui::Widgets::Metrics
{
    /** Retourne une couleur verte standard pour un indicateur performant. */
    inline ImVec4 GoodColor() noexcept { return ThemeColors::PerformanceGood; }

    /** Retourne une couleur jaune standard pour un indicateur attention. */
    inline ImVec4 WarningColor() noexcept { return ThemeColors::PerformanceWarning; }

    /** Retourne une couleur rouge standard pour un indicateur critique. */
    inline ImVec4 CriticalColor() noexcept { return ThemeColors::PerformanceCritical; }

    /**
     * \brief Détermine automatiquement la couleur à utiliser pour un FPS donné.
     * \param fps Valeur du framerate courant.
     * \param warningThreshold Seuil d'avertissement (par défaut 60 fps).
     * \param criticalThreshold Seuil critique (par défaut 30 fps).
     */
    inline ImVec4 ColorForFps(float fps, float warningThreshold = 60.0f, float criticalThreshold = 30.0f) noexcept
    {
        if (fps < criticalThreshold)
            return CriticalColor();
        if (fps < warningThreshold)
            return WarningColor();
        return GoodColor();
    }
}
