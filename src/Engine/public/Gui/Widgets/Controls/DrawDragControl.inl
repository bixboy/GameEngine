#pragma once

#include <type_traits>

namespace BixEngine::Gui::Widgets
{
    template <typename T>
    bool DrawDragControl(const char* label, T& value, float speed, const void* minValue, const void* maxValue, const char* format)
    {
        ImGuiDataType dataType{};

        if constexpr (std::is_same_v<T, int>)
        {
            dataType = ImGuiDataType_S32;
        }
        else if constexpr (std::is_same_v<T, unsigned int>)
        {
            dataType = ImGuiDataType_U32;
        }
        else if constexpr (std::is_same_v<T, long long>)
        {
            dataType = ImGuiDataType_S64;
        }
        else if constexpr (std::is_same_v<T, unsigned long long>)
        {
            dataType = ImGuiDataType_U64;
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            dataType = ImGuiDataType_Float;
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            dataType = ImGuiDataType_Double;
        }
        else
        {
            static_assert(std::is_same_v<T, void>, "DrawDragControl ne prend pas en charge ce type.");
            return false;
        }

        return ImGui::DragScalar(label, dataType, &value, speed, minValue, maxValue, format);
    }
}
