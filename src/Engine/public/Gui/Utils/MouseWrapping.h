#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#include "imgui.h"

namespace BixEngine::Gui::Utils
{
    /**
     * \brief Applique le mouse wrapping pour les opérations de drag actives.
     * 
     * Cette fonction doit être appelée immédiatement après un widget ImGui qui supporte le drag
     * (DragFloat, DragInt, ColorEdit, etc.) pour permettre au curseur de la souris de "wrapper"
     * d'un bord de l'écran à l'autre pendant les opérations de drag, permettant des mouvements illimités.
     * 
     * La fonction vérifie si le widget précédent est actif et, si le curseur atteint un bord de l'écran
     * (avec une marge de 10 pixels), le repositionne du côté opposé tout en mettant à jour la position
     * interne d'ImGui pour éviter les sauts.
     * 
     * Cette fonctionnalité est uniquement disponible sur Windows.
     */
    inline void ApplyMouseWrapping()
    {
#ifdef _WIN32
        if (ImGui::IsItemActive())
        {
            POINT cursorPos;
            if (::GetCursorPos(&cursorPos))
            {
                const int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
                const int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
                const int margin = 10;

                bool wrapped = false;
                POINT newPos = cursorPos;

                // Horizontal wrapping
                if (cursorPos.x <= margin)
                {
                    newPos.x = screenWidth - margin - 1;
                    wrapped = true;
                }
                else if (cursorPos.x >= screenWidth - margin)
                {
                    newPos.x = margin + 1;
                    wrapped = true;
                }

                // Vertical wrapping
                if (cursorPos.y <= margin)
                {
                    newPos.y = screenHeight - margin - 1;
                    wrapped = true;
                }
                else if (cursorPos.y >= screenHeight - margin)
                {
                    newPos.y = margin + 1;
                    wrapped = true;
                }

                // Apply wrapping if needed
                if (wrapped)
                {
                    ::SetCursorPos(newPos.x, newPos.y);

                    // Update ImGui's mouse position to match
                    ImGuiIO& io = ImGui::GetIO();
                    io.MousePos.x += static_cast<float>(newPos.x - cursorPos.x);
                    io.MousePos.y += static_cast<float>(newPos.y - cursorPos.y);
                }
            }
        }
#endif
    }
}
