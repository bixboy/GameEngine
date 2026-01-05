#pragma once
#include "imgui.h"


namespace BixEngine::Gui::Widgets::Styling
{
    class ScopedColor
    {
    public:
        ScopedColor(ImGuiCol colorIndex, const ImVec4& color) noexcept;


        ScopedColor(ImGuiCol colorIndex, ImU32 color) noexcept;

        ~ScopedColor();

        ScopedColor(const ScopedColor&) = delete;
        ScopedColor& operator=(const ScopedColor&) = delete;
        ScopedColor(ScopedColor&&) = delete;
        ScopedColor& operator=(ScopedColor&&) = delete;

    private:
        bool active_{false};
    };
}