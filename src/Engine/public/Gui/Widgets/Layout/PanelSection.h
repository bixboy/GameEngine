#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    /**
     * \brief RAII pour ImGui::CollapsingHeader, avec gestion automatique de l'espacement final.
     *
     * Utiliser PanelSection pour encapsuler le contenu d'une section repliable :
     * \code
     * PanelSection section("Transform");
     * if (section.IsOpen())
     * {
     *     // ... contenu ...
     * }
     * \endcode
     */
    class PanelSection
    {
    public:
        PanelSection(const char* label, bool defaultOpen = true, ImGuiTreeNodeFlags flags = 0);
        ~PanelSection();

        PanelSection(const PanelSection&) = delete;
        PanelSection& operator=(const PanelSection&) = delete;
        PanelSection(PanelSection&&) = delete;
        PanelSection& operator=(PanelSection&&) = delete;

        [[nodiscard]] bool IsOpen() const noexcept { return open_; }

    private:
        bool open_{false};
    };
}
