#pragma once
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Widgets/Styling/ScopedColor.h"
#include "Gui/Widgets/Styling/ScopedStyle.h"
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
     
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
