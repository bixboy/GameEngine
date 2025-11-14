#pragma once
#include "Gui/Widgets/Layout/PanelSection.h"

namespace BixEngine::Gui::Widgets
{
    inline namespace Builder
    {
        /**
         * \brief Helper RAII pour créer des sections ImGui collapsibles avec une syntaxe fluide.
         */
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
}
