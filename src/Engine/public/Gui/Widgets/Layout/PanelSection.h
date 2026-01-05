#pragma once


namespace BixEngine::Gui::Widgets::Layout
{
    class PanelSection
    {
    public:
        PanelSection(const char* label, bool defaultOpen = true);
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