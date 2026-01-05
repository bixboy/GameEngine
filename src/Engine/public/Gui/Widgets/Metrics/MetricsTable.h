#pragma once
#include <span>
#include <string>

#include "imgui.h"


namespace BixEngine::Gui::Widgets
{
    class MetricsTable
    {
    public:

        struct MetricDisplay
        {
            std::string label;
            std::string value;
            ImVec4 color{1.0f, 1.0f, 1.0f, 1.0f};
            std::string tooltip;

            MetricDisplay(std::string l, std::string v, ImVec4 c = {1,1,1,1}, std::string t = "")
                : label(std::move(l)), value(std::move(v)), color(c), tooltip(std::move(t)) 
            {}
        };
        
        static bool Begin(const char* id, float labelColumnWidth = 140.0f);
        
        static void End();

        static void Draw(const char* label, const char* value, const ImVec4& color = ImVec4(1,1,1,1), const char* tooltip = nullptr);
        
        static void DrawFloat(const char* label, float value, const char* format = "%.2f", const ImVec4* colorOverride = nullptr, const char* tooltip = nullptr);

        static void DrawInt(const char* label, int value, const ImVec4& color = ImVec4(1,1,1,1), const char* tooltip = nullptr);

        static void DrawMetricsTable(std::span<const MetricDisplay> metrics, float labelColumnWidth);
    };
}