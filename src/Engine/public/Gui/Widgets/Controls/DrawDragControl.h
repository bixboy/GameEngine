#pragma once
#include "imgui.h"
#include "Gui/Utils/MouseWrapping.h"
#include <type_traits> // Nécessaire pour std::is_same_v

namespace BixEngine::Gui::Widgets
{
    template<typename T> struct always_false : std::false_type {};

    template <typename T>
    bool DrawDragControl(const char* label, T& value, float speed = 1.0f, const void* minValue = nullptr,
        const void* maxValue = nullptr, const char* format = nullptr)
    {
        ImGuiDataType dataType;
        const char* currentFormat = format;

        if constexpr (std::is_same_v<T, int>)
        {
            dataType = ImGuiDataType_S32;
            if (!currentFormat)
                currentFormat = "%d";
        }
        else if constexpr (std::is_same_v<T, unsigned int>)
        {
            dataType = ImGuiDataType_U32;
            if (!currentFormat)
                currentFormat = "%d";
        }
        else if constexpr (std::is_same_v<T, long long>)
        {
            dataType = ImGuiDataType_S64;
            if (!currentFormat)
                currentFormat = "%lld";
        }
        else if constexpr (std::is_same_v<T, unsigned long long>)
        {
            dataType = ImGuiDataType_U64;
            if (!currentFormat)
                currentFormat = "%llu";
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            dataType = ImGuiDataType_Float;
            if (!currentFormat)
                currentFormat = "%.3f";
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            dataType = ImGuiDataType_Double;
            if (!currentFormat)
                currentFormat = "%.3f";
        }
        else
        {
            static_assert(always_false<T>::value, "DrawDragControl: Type non supporté");
            return false;
        }

        bool result = ImGui::DragScalar(label, dataType, &value, speed, minValue, maxValue, currentFormat);
        
        Utils::ApplyMouseWrapping();
        
        return result;
    }
}