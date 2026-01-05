#pragma once
#include "Gui/Widgets/ImGuiScopeBase.h"

namespace BixEngine::Gui::Widgets::Builder
{
     
    class Menu : ImGuiScopeBase
    {
    public:
        Menu(const char* label, bool enabled = true) noexcept;
        ~Menu();

        [[nodiscard]] bool IsOpen() const noexcept { return IsActive(); } 
    
        operator bool() const noexcept { return IsActive(); }
    };
}
