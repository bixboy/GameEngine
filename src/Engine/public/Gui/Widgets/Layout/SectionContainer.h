#pragma once
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Widgets/Styling/ScopedColor.h"
#include "Gui/Widgets/Styling/ScopedStyle.h"


namespace BixEngine::Gui::Widgets::Layout
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
        GuiUtils::ScopedID idScope_;
        Styling::ScopedColor background_;
        Styling::ScopedStyle rounding_;
        Styling::ScopedStyle padding_;
        bool isVisible_{false};
    };
}