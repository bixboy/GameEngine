#pragma once
#include "Gui/GuiTheme.h"
#include "Gui/Utils/GuiHelpers.h"
#include <imgui.h>
#include <string>
#include <type_traits>


namespace BixEngine::Gui::ActorInspector
{
    ImVec2 DrawBadge(const char* label, const ImVec4& backgroundColor, const ImVec4& textColor);
    bool DrawVector3Control(const char* label, float* values, float resetValue, float speed, const char* format);

    template <typename T>
    bool DrawDragControl(const char* label, T& value, float speed = 1.0f, const void* minValue = nullptr,
                         const void* maxValue = nullptr, const char* format = nullptr)
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
        PersistentSectionScope(const char* label, const std::string& contextId, bool defaultOpen = true,
                               ImGuiTreeNodeFlags flags = 0);
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

        [[nodiscard]] bool IsVisible() const noexcept { return isVisible_; }

    private:
        Utils::ScopedID idScope_;
        Utils::ScopedColor background_;
        Utils::ScopedStyle rounding_;
        Utils::ScopedStyle padding_;
        bool isVisible_{false};
    };
}
