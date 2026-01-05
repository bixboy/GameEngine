#pragma once
#include "imgui.h"


namespace BixEngine::Gui::Widgets::Styling
{
    class ScopedStyle
    {
    public:
        ScopedStyle(ImGuiStyleVar variable, float value) noexcept;
        ScopedStyle(ImGuiStyleVar variable, const ImVec2& value) noexcept;

        ~ScopedStyle();

        ScopedStyle(const ScopedStyle&) = delete;
        ScopedStyle& operator=(const ScopedStyle&) = delete;
        ScopedStyle(ScopedStyle&&) = delete;
        ScopedStyle& operator=(ScopedStyle&&) = delete;

    private:
        bool active_{false};
    };
}