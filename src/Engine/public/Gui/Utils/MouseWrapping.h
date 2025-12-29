#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#include "imgui.h"

namespace BixEngine::Gui::Utils
{
     
    inline void ApplyMouseWrapping()
    {
#ifdef _WIN32
        if (ImGui::IsItemActive())
        {
            POINT cursorPos;
            if (::GetCursorPos(&cursorPos))
            {
                const int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
                const int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
                const int margin = 10;

                bool wrapped = false;
                POINT newPos = cursorPos;

                
                if (cursorPos.x <= margin)
                {
                    newPos.x = screenWidth - margin - 1;
                    wrapped = true;
                }
                else if (cursorPos.x >= screenWidth - margin)
                {
                    newPos.x = margin + 1;
                    wrapped = true;
                }

                
                if (cursorPos.y <= margin)
                {
                    newPos.y = screenHeight - margin - 1;
                    wrapped = true;
                }
                else if (cursorPos.y >= screenHeight - margin)
                {
                    newPos.y = margin + 1;
                    wrapped = true;
                }

                
                if (wrapped)
                {
                    ::SetCursorPos(newPos.x, newPos.y);

                    
                    ImGuiIO& io = ImGui::GetIO();
                    io.MousePos.x += static_cast<float>(newPos.x - cursorPos.x);
                    io.MousePos.y += static_cast<float>(newPos.y - cursorPos.y);
                }
            }
        }
#endif
    }
}
