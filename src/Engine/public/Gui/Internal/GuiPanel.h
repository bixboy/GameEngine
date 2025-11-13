#pragma once

#include <functional>
#include "Containers/String.h"
#include "Gui/GuiDocking.h"
#include "imgui.h"

namespace BixEngine::Gui
{
    /**
     * @brief Représente une fenêtre ImGui indépendante.
     */
    class GuiPanel
    {
    public:
        using DrawFunction = std::function<void()>;

        GuiPanel(String name, String title);

        // ────────────────────────────────────────────────
        // ⚙️ Configuration de base
        // ────────────────────────────────────────────────

        void SetTitle(String title);
        [[nodiscard]] const String& GetTitle() const noexcept { return title_; }
        [[nodiscard]] const String& GetName() const noexcept { return name_; }

        void SetVisible(bool visible) noexcept { visible_ = visible; }
        [[nodiscard]] bool IsVisible() const noexcept { return visible_; }

        // ────────────────────────────────────────────────
        // 🧩 Position et taille
        // ────────────────────────────────────────────────

        void SetPosition(float x, float y, ImGuiCond condition = ImGuiCond_Always) noexcept;
        void ResetPosition() noexcept { usePosition_ = false; }

        void SetSize(float width, float height, ImGuiCond condition = ImGuiCond_Always) noexcept;
        void ResetSize() noexcept { useSize_ = false; }

        [[nodiscard]] ImVec2 GetPosition() const noexcept { return windowPos_; }
        [[nodiscard]] ImVec2 GetSize() const noexcept { return windowSize_; }

        // ────────────────────────────────────────────────
        // 🪟 Apparence et comportement
        // ────────────────────────────────────────────────

        void SetResizable(bool resizable) noexcept { resizable_ = resizable; }
        void SetMovable(bool movable) noexcept { movable_ = movable; }
        void SetClosable(bool closable) noexcept { closable_ = closable; }
        void SetCollapsable(bool collapsable) noexcept { collapsable_ = collapsable; }

        void SetWindowFlags(ImGuiWindowFlags flags) noexcept { windowFlags_ = flags; }
        void AddWindowFlags(ImGuiWindowFlags flags) noexcept { windowFlags_ |= flags; }
        void RemoveWindowFlags(ImGuiWindowFlags flags) noexcept { windowFlags_ &= ~flags; }
        [[nodiscard]] ImGuiWindowFlags GetWindowFlags() const noexcept { return windowFlags_; }

        // ────────────────────────────────────────────────
        // 🎨 Style local
        // ────────────────────────────────────────────────

        struct PanelStyle
        {
            bool override{false};
            ImVec4 bgColor{1.f, 1.f, 1.f, 1.f};
            float rounding{0.f};
            float border{0.f};
        };

        void SetPanelStyle(const PanelStyle& style) noexcept { style_ = style; }
        [[nodiscard]] const PanelStyle& GetPanelStyle() const noexcept { return style_; }

        void SetBackgroundColor(const ImVec4& color) noexcept;
        void ResetBackgroundColor() noexcept { useBackgroundColor_ = false; }

        // ────────────────────────────────────────────────
        // 🔗 Docking
        // ────────────────────────────────────────────────

        void SetDockingPreference(DockSpaceRegion area, ImGuiCond condition = ImGuiCond_FirstUseEver) noexcept;
        void ResetDockingPreference() noexcept;
        [[nodiscard]] bool HasDockingPreference() const noexcept { return dockPreferenceSet_; }
        [[nodiscard]] DockSpaceRegion GetDockingPreference() const noexcept { return dockPreference_; }
        [[nodiscard]] ImGuiCond GetDockingPreferenceCondition() const noexcept { return dockPreferenceCondition_; }

        void SetDockId(ImGuiID dockId, ImGuiCond condition, ImGuiCond fallbackCondition) noexcept;
        void ResetDockId() noexcept;

        // ────────────────────────────────────────────────
        // 🧠 Cycle de vie et rendu
        // ────────────────────────────────────────────────

        void SetDrawFunction(DrawFunction drawFunction);
        void Draw();

        // ────────────────────────────────────────────────
        // 🎮 Callbacks d’événements
        // ────────────────────────────────────────────────

        std::function<void()> OnOpen;
        std::function<void()> OnClose;
        std::function<void()> OnFocus;
        std::function<void()> OnUnfocus;

        void SetContextMenu(std::function<void()> fn) noexcept { contextMenu_ = std::move(fn); }

        // ────────────────────────────────────────────────
        // 🏗️ Mode de panneau (éditeur/runtime)
        // ────────────────────────────────────────────────

        enum class GuiPanelMode { EditorOnly, RuntimeOnly, Both };

        void SetMode(GuiPanelMode mode) noexcept { mode_ = mode; }
        [[nodiscard]] GuiPanelMode GetMode() const noexcept { return mode_; }

        // ────────────────────────────────────────────────
        // 🧾 Utilitaires
        // ────────────────────────────────────────────────

        [[nodiscard]] bool IsFocused() const noexcept;
        [[nodiscard]] bool IsHovered() const noexcept;

    private:
        String name_;
        String title_;
        bool visible_{true};

        // Position / taille
        bool usePosition_{false};
        ImVec2 position_{0.0f, 0.0f};
        ImGuiCond positionCondition_{ImGuiCond_Always};

        bool useSize_{false};
        ImVec2 size_{0.0f, 0.0f};
        ImGuiCond sizeCondition_{ImGuiCond_Always};

        // Apparence
        bool resizable_{true};
        bool movable_{true};
        bool closable_{false};
        bool collapsable_{false};
        ImGuiWindowFlags windowFlags_{ImGuiWindowFlags_None};

        bool useBackgroundColor_{false};
        ImVec4 backgroundColor_{1.f, 1.f, 1.f, 1.f};

        PanelStyle style_{};

        // Docking
        bool dockPreferenceSet_{true};
        DockSpaceRegion dockPreference_{DockSpaceRegion::Center};
        ImGuiCond dockPreferenceCondition_{ImGuiCond_FirstUseEver};

        bool useDockId_{false};
        bool applyDockFallback_{false};
        ImGuiID dockId_{0};
        ImGuiCond dockCondition_{ImGuiCond_FirstUseEver};
        ImGuiCond dockFallbackCondition_{ImGuiCond_FirstUseEver};

        // Contenu et callbacks
        DrawFunction drawFunction_{};
        std::function<void()> contextMenu_;

        // État runtime
        ImVec2 windowPos_{0.0f, 0.0f};
        ImVec2 windowSize_{0.0f, 0.0f};
        GuiPanelMode mode_{GuiPanelMode::Both};
    };
}
