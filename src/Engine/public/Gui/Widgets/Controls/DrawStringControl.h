#pragma once
#include <string>
#include "imgui.h"


namespace BixEngine::Gui::Widgets
{
    bool DrawStringControl(const char* label, std::string& value, float columnWidth = 120.0f, ImGuiInputTextFlags flags = 0);
}