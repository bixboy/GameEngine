#pragma once
#include "Gui/Widgets/Internal/ImGuiScopeBase.h"
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
    class ScopedStyle : Internal::ImGuiScopeBase
    {
    public:
        ScopedStyle(ImGuiStyleVar variable, float value) noexcept;
        ScopedStyle(ImGuiStyleVar variable, const ImVec2& value) noexcept;
        ~ScopedStyle();

        ScopedStyle(const ScopedStyle&) = delete;
        ScopedStyle& operator=(const ScopedStyle&) = delete;
        ScopedStyle(ScopedStyle&&) = delete;
        ScopedStyle& operator=(ScopedStyle&&) = delete;

    };
}
