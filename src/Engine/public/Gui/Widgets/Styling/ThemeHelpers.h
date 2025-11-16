#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets::ThemeHelpers
{
    /**
     * \brief Applique un facteur de transparence sur une couleur ImGui.
     * \param color Couleur source.
     * \param alpha Nouvel alpha (0..1).
     */
    ImVec4 WithAlpha(const ImVec4& color, float alpha) noexcept;

    /**
     * \brief Éclaircit une couleur en ajoutant un delta uniforme sur chaque composante RGB.
     * \param color Couleur source.
     * \param delta Intensité à ajouter (valeurs positives pour éclaircir, négatives pour assombrir).
     */
    ImVec4 AdjustColor(const ImVec4& color, float delta) noexcept;

    /**
     * \brief Convertit une ImVec4 en couleur 32 bits ImGui.
     */
    ImU32 ToColor32(const ImVec4& color) noexcept;
}
