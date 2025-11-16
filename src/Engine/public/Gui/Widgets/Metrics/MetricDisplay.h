#pragma once
#include <string>
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    /**
     * \brief Représente une ligne de métrique (label + valeur + couleur + info-bulle optionnelle).
     */
    struct MetricDisplay
    {
        std::string label{};
        std::string value{};
        ImVec4 color{1.0f, 1.0f, 1.0f, 1.0f};
        std::string hint{};

        MetricDisplay() = default;
        MetricDisplay(std::string label, std::string value, ImVec4 color, std::string hint = {});
    };
}
