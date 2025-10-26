#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "Bix/Core/String.h"

#include "Bix/Engine/Gui/GuiDocking.h"

#include "imgui.h"

namespace BixEngine::Gui
{
    class GuiSystem;
    class GuiPanel;

    class GuiManager
    {
        public:
            explicit GuiManager(GuiSystem& guiSystem);
            ~GuiManager();

            GuiPanel& CreatePanel(String name, String title);
            void RemovePanel(const String& name);

            [[nodiscard]] GuiPanel* FindPanel(const String& name) noexcept;
            [[nodiscard]] const GuiPanel* FindPanel(const String& name) const noexcept;

            void SetPanelDockingArea(const String& name, DockSpaceRegion area, ImGuiCond condition = ImGuiCond_FirstUseEver);
            void SetPanelDockingArea(GuiPanel& panel, DockSpaceRegion area, ImGuiCond condition = ImGuiCond_FirstUseEver);

            void DrawAll();

            [[nodiscard]] std::vector<GuiPanel*> GetPanels();
            [[nodiscard]] std::vector<const GuiPanel*> GetPanels() const;

        private:
            GuiSystem* guiSystem_{nullptr};
            std::unordered_map<String, std::unique_ptr<GuiPanel>> panels_{};
    };
}
