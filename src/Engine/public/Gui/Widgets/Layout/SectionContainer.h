#pragma once
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Widgets/Styling/ScopedColor.h"
#include "Gui/Widgets/Styling/ScopedStyle.h"
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    /**
     * \brief Conteneur enfant stylisé pour encapsuler une section avec fond et marges.
     *
     * Crée un ImGui::BeginChild configuré pour s'adapter verticalement au contenu et applique
     * les styles de fond/padding nécessaires pour harmoniser les panneaux de l'éditeur.
     */
    class SectionContainer
    {
    public:
        explicit SectionContainer(const char* id);
        ~SectionContainer();

        SectionContainer(const SectionContainer&) = delete;
        SectionContainer& operator=(const SectionContainer&) = delete;
        SectionContainer(SectionContainer&&) = delete;
        SectionContainer& operator=(SectionContainer&&) = delete;

        [[nodiscard]] bool IsVisible() const noexcept { return isVisible_; }

    private:
        Gui::Utils::ScopedID idScope_;
        ScopedColor background_;
        ScopedStyle rounding_;
        ScopedStyle padding_;
        bool isVisible_{false};
    };
}
