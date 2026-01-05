#pragma once

namespace BixEngine::Gui::Widgets
{
    // Float
    bool DrawVector3Control(const char* label, float* values, float resetValue = 0.0f, float speed = 0.1f, const char* format = "%.3f");

    // Int
    bool DrawVector3Control(const char* label, int* values, int resetValue = 0, float speed = 1.0f, const char* format = "%d");
}
