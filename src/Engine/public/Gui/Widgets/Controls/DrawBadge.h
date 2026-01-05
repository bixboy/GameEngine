#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
     
    ImVec2 DrawBadge(const char* label, const ImVec4& backgroundColor, const ImVec4& textColor);

    inline ImVec2 DrawBadge(const char* label)
    {
        return DrawBadge(label, ImVec4(0.3f, 0.3f, 0.3f, 1.0f), ImVec4(1,1,1,1));
    }
}
