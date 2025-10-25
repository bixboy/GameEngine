#pragma once

#include "Bix/Engine/Gui/Utils/GuiHelpers.h"

#include <cfloat>
#include <imgui.h>
#include <string>
#include <type_traits>

namespace BixEngine::Gui::ActorInspector
{
    namespace Colors
    {
        inline constexpr ImVec4 kInspectorBackground{0.12f, 0.12f, 0.12f, 0.95f};
        inline constexpr ImVec4 kSectionBackground{0.18f, 0.18f, 0.18f, 0.65f};
        inline constexpr ImVec4 kOverviewBackground{0.16f, 0.16f, 0.19f, 0.85f};
        inline constexpr ImVec4 kAxisXColor{0.80f, 0.28f, 0.28f, 1.0f};
        inline constexpr ImVec4 kAxisYColor{0.32f, 0.72f, 0.45f, 1.0f};
        inline constexpr ImVec4 kAxisZColor{0.26f, 0.45f, 0.86f, 1.0f};
    }

    ImVec4 AdjustColor(const ImVec4& color, float delta);
    ImVec2 DrawBadge(const char* label, const ImVec4& backgroundColor, const ImVec4& textColor);
    bool DrawVector3Control(const char* label, float* values, float resetValue, float speed, const char* format);

    template <typename T>
    bool DrawDragControl(const char* label, T& value, float speed = 1.0f, const T* minValue = nullptr, const T* maxValue = nullptr, const char* format = nullptr)
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
            static_assert(std::is_same_v<T, void>, "DrawDragControl does not support this type");
            return false;
        }

        return ImGui::DragScalar(label, dataType, &value, speed, minValue, maxValue, format);
    }

    class PersistentSectionScope
    {
    public:
        PersistentSectionScope(const char* label, const std::string& contextId, bool defaultOpen = true, ImGuiTreeNodeFlags flags = 0);
        ~PersistentSectionScope();

        [[nodiscard]] bool IsOpen() const noexcept { return isOpen_; }

    private:
        bool isOpen_{false};
    };

    class SectionContainer
    {
    public:
        explicit SectionContainer(const char* id);
        ~SectionContainer();

        [[nodiscard]] bool IsVisible() const noexcept { return true; }
    };
}

