#pragma once
#include "imgui.h"


namespace BixEngine::Gui::Widgets
{
    class MetricsTable
    {
    public:
        static bool Begin(const char* id, float labelColumnWidth = 140.0f);
        
        static void End();

        static void Draw(const char* label, const char* value, const ImVec4& color = ImVec4(1,1,1,1), const char* tooltip = nullptr);
        
        static void DrawFloat(const char* label, float value, const char* format = "%.2f", const ImVec4* colorOverride = nullptr, const char* tooltip = nullptr);

        static void DrawInt(const char* label, int value, const ImVec4& color = ImVec4(1,1,1,1), const char* tooltip = nullptr);
    };
}