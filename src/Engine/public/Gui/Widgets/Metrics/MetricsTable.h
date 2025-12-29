#pragma once
#include <vector>

#include "Gui/Widgets/Metrics/MetricDisplay.h"

namespace BixEngine::Gui::Widgets
{
     
    void DrawMetricRow(const MetricDisplay& metric);

     
    void DrawMetricsTable(const std::vector<MetricDisplay>& metrics, float columnWidth = 140.0f,
                          const char* id = "MetricsTable");
}
