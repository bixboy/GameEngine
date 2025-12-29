#pragma once
#include "Gui/Widgets/Layout/PanelSection.h"

namespace BixEngine::Gui::Widgets
{
     
    class Section
    {
    public:
        Section(const char* label, bool defaultOpen = true, ImGuiTreeNodeFlags flags = 0);
        ~Section();

        Section(const Section&) = delete;
        Section& operator=(const Section&) = delete;
        
        Section(Section&&) = delete;
        Section& operator=(Section&&) = delete;

        [[nodiscard]] bool IsOpen() const noexcept { return section_.IsOpen(); }

    private:
        PanelSection section_;
    };
}
