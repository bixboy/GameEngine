#pragma once
#include "Gui/Widgets/Styling/ThemeColors.h"

namespace BixEngine::Gui::Widgets::Metrics
{
     
    inline ImVec4 GoodColor() noexcept { return ThemeColors::PerformanceGood; }

     
    inline ImVec4 WarningColor() noexcept { return ThemeColors::PerformanceWarning; }

     
    inline ImVec4 CriticalColor() noexcept { return ThemeColors::PerformanceCritical; }

     
    inline ImVec4 ColorForFps(float fps, float warningThreshold = 60.0f, float criticalThreshold = 30.0f) noexcept
    {
        if (fps < criticalThreshold)
            return CriticalColor();
        if (fps < warningThreshold)
            return WarningColor();
        return GoodColor();
    }
}
