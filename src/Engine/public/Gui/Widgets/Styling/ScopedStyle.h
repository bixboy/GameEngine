#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    /**
     * \brief Gestion RAII d'un appel ImGui::PushStyleVar/PopStyleVar.
     *
     * Cette classe garantit que le style poussé sur la pile est toujours relâché,
     * même en cas de retour anticipé. Utiliser une instance sur la pile pour chaque
     * personnalisation de style temporaire (padding, spacing, etc.).
     */
    class ScopedStyle
    {
    public:
        ScopedStyle(ImGuiStyleVar variable, float value) noexcept;
        ScopedStyle(ImGuiStyleVar variable, const ImVec2& value) noexcept;
        ~ScopedStyle();

        ScopedStyle(const ScopedStyle&) = delete;
        ScopedStyle& operator=(const ScopedStyle&) = delete;
        ScopedStyle(ScopedStyle&&) = delete;
        ScopedStyle& operator=(ScopedStyle&&) = delete;

    private:
        bool engaged_{false};
    };
}
