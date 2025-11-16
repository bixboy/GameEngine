#include "Gui/Widgets/Metrics/MetricDisplay.h"

#include <utility>

namespace BixEngine::Gui::Widgets
{
    MetricDisplay::MetricDisplay(std::string labelValue, std::string formattedValue, ImVec4 displayColor, std::string tooltip)
        : label(std::move(labelValue))
        , value(std::move(formattedValue))
        , color(displayColor)
        , hint(std::move(tooltip))
    {
    }
}
