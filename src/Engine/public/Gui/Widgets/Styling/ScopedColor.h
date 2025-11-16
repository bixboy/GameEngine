#pragma once
#include "Gui/Widgets/Internal/ImGuiScopeBase.h"
#include "imgui.h"


namespace BixEngine::Gui::Widgets
{
    /**
     * \brief Gestion RAII d'un ImGui::PushStyleColor/PopStyleColor.
     *
     * Utiliser cette classe pour appliquer temporairement une couleur à un contrôle.
     * Les PopStyleColor nécessaires sont automatiquement effectués à la destruction.
     */
    class ScopedColor : Internal::ImGuiScopeBase
    {
    public:
        ScopedColor(ImGuiCol colorIndex, const ImVec4& color) noexcept;
        ~ScopedColor();

        ScopedColor(const ScopedColor&) = delete;
        ScopedColor& operator=(const ScopedColor&) = delete;
        
        ScopedColor(ScopedColor&&) = delete;
        ScopedColor& operator=(ScopedColor&&) = delete;

    };
}
