#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
     
    template <typename T>
    bool DrawDragControl(const char* label, T& value, float speed = 1.0f, const void* minValue = nullptr,
        const void* maxValue = nullptr, const char* format = nullptr);
}

#include "DrawDragControl.inl"
