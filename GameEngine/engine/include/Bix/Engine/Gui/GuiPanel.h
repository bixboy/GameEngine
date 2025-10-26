#pragma once

#include <functional>

#include "Bix/Core/String.h"

#include "Bix/Engine/Gui/GuiDocking.h"

#include "imgui.h"

namespace BixEngine::Gui
{
    class GuiPanel
    {
        public:
            using DrawFunction = std::function<void()>;

            GuiPanel(String name, String title);

            void SetTitle(String title);
            [[nodiscard]] const String& GetTitle() const noexcept { return title_; }
            [[nodiscard]] const String& GetName() const noexcept { return name_; }

            void SetVisible(bool visible) noexcept { visible_ = visible; }
            [[nodiscard]] bool IsVisible() const noexcept { return visible_; }

            void SetPosition(float x, float y, ImGuiCond condition = ImGuiCond_Always) noexcept;
            void ResetPosition() noexcept { usePosition_ = false; }

            void SetSize(float width, float height, ImGuiCond condition = ImGuiCond_Always) noexcept;
            void ResetSize() noexcept { useSize_ = false; }

            void SetResizable(bool resizable) noexcept { resizable_ = resizable; }
            void SetMovable(bool movable) noexcept { movable_ = movable; }
            void SetClosable(bool closable) noexcept { closable_ = closable; }
            void SetCollapsable(bool collapsable) noexcept { collapsable_ = collapsable; }

            void SetWindowFlags(ImGuiWindowFlags flags) noexcept { windowFlags_ = flags; }
            void AddWindowFlags(ImGuiWindowFlags flags) noexcept { windowFlags_ |= flags; }
            void RemoveWindowFlags(ImGuiWindowFlags flags) noexcept { windowFlags_ &= ~flags; }
            [[nodiscard]] ImGuiWindowFlags GetWindowFlags() const noexcept { return windowFlags_; }

            void SetBackgroundColor(const ImVec4& color) noexcept;
            void ResetBackgroundColor() noexcept { useBackgroundColor_ = false; }

            void SetDrawFunction(DrawFunction drawFunction);

            void SetDockingPreference(DockSpaceRegion area, ImGuiCond condition = ImGuiCond_FirstUseEver) noexcept;
            void ResetDockingPreference() noexcept;
            [[nodiscard]] bool HasDockingPreference() const noexcept { return dockPreferenceSet_; }
            [[nodiscard]] DockSpaceRegion GetDockingPreference() const noexcept { return dockPreference_; }
            [[nodiscard]] ImGuiCond GetDockingPreferenceCondition() const noexcept { return dockPreferenceCondition_; }

            void SetDockId(ImGuiID dockId, ImGuiCond condition, ImGuiCond fallbackCondition) noexcept;
            void ResetDockId() noexcept;

            void Draw();

        private:
            String name_;
            String title_;
            bool visible_{true};

            bool usePosition_{false};
            ImVec2 position_{0.0f, 0.0f};
            ImGuiCond positionCondition_{ImGuiCond_Always};

            bool useSize_{false};
            ImVec2 size_{0.0f, 0.0f};
            ImGuiCond sizeCondition_{ImGuiCond_Always};

            bool resizable_{true};
            bool movable_{true};
            bool closable_{false};
            bool collapsable_{false};

            ImGuiWindowFlags windowFlags_{ImGuiWindowFlags_None};

            bool useBackgroundColor_{false};
            ImVec4 backgroundColor_{1.0f, 1.0f, 1.0f, 1.0f};

            bool dockPreferenceSet_{true};
            DockSpaceRegion dockPreference_{DockSpaceRegion::Center};
            ImGuiCond dockPreferenceCondition_{ImGuiCond_FirstUseEver};

            bool useDockId_{false};
            bool applyDockFallback_{false};
            ImGuiID dockId_{0};
            ImGuiCond dockCondition_{ImGuiCond_FirstUseEver};
            ImGuiCond dockFallbackCondition_{ImGuiCond_FirstUseEver};

            DrawFunction drawFunction_{};
    };
}
