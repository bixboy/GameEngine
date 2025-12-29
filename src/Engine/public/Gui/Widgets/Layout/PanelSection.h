#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
     
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
