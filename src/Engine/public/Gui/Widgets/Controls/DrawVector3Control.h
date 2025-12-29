#pragma once
#include "imgui.h"

namespace BixEngine::Gui::Widgets
{
     
    bool DrawVector3Control(const char* label, float* values, float resetValue, float speed, const char* format);
}
