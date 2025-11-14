#pragma once
#include <vector>

#include "Gui/Widgets/Metrics/MetricDisplay.h"

namespace BixEngine::Gui::Widgets
{
    /**
     * \brief Dessine une ligne de métrique dans un tableau ImGui déjà ouvert.
     */
    void DrawMetricRow(const MetricDisplay& metric);

    /**
     * \brief Dessine un tableau complet de métriques (label + valeur colorée).
     * \param metrics Collection de métriques à afficher.
     * \param columnWidth Largeur de la colonne des labels.
     * \param id Identifiant unique pour l'instance de tableau (éviter les conflits ImGui).
     */
    void DrawMetricsTable(const std::vector<MetricDisplay>& metrics, float columnWidth = 140.0f,
                          const char* id = "MetricsTable");
}
