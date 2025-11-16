#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    /**
     * \brief Contrôle de type DragScalar générique prenant en charge les types courants ImGui.
     * \param label Libellé ImGui.
     * \param value Référence vers la valeur manipulée.
     * \param speed Vitesse de variation.
     * \param minValue Valeur minimale optionnelle (pointeur vers la donnée de même type).
     * \param maxValue Valeur maximale optionnelle.
     * \param format Chaîne de format (utilise le format par défaut d'ImGui si nullptr).
     */
    template <typename T>
    bool DrawDragControl(const char* label, T& value, float speed = 1.0f, const void* minValue = nullptr,
                         const void* maxValue = nullptr, const char* format = nullptr);
}

#include "DrawDragControl.inl"
