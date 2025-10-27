#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <utility>

#include "Core/Containers/String.h"

#include "Engine/Gui/GuiDocking.h"

#include "imgui.h"

namespace BixEngine::Gui
{
    class GuiSystem;
    class GuiPanel;
    class GuiPanelController;

    template <typename ControllerT>
    struct PanelRegistration
    {
        GuiPanel& panel;
        ControllerT& controller;
    };

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

            GuiPanelController& AttachController(const String& name, std::unique_ptr<GuiPanelController> controller);
            GuiPanelController& AttachController(GuiPanel& panel, std::unique_ptr<GuiPanelController> controller);
            void DetachController(const String& name);
            void DetachController(GuiPanel& panel);

            GuiPanelController* GetController(const String& name) noexcept;
            const GuiPanelController* GetController(const String& name) const noexcept;

            template <typename ControllerT, typename... Args>
            PanelRegistration<ControllerT> RegisterUtilityPanel(String name, String title, Args&&... args)
            {
                GuiPanel& panel = CreatePanel(std::move(name), std::move(title));
                auto controller = std::make_unique<ControllerT>(std::forward<Args>(args)...);
                ControllerT& controllerRef = static_cast<ControllerT&>(AttachController(panel, std::move(controller)));
                return PanelRegistration<ControllerT>{panel, controllerRef};
            }

        private:
            struct PanelEntry
            {
                std::unique_ptr<GuiPanel> panel;
                std::unique_ptr<GuiPanelController> controller;
            };

            [[nodiscard]] PanelEntry* FindPanelEntry(const String& name) noexcept;
            [[nodiscard]] PanelEntry* FindPanelEntry(GuiPanel& panel) noexcept;
            [[nodiscard]] const PanelEntry* FindPanelEntry(const String& name) const noexcept;
            void AttachDrawFunction_(PanelEntry& entry);

            GuiSystem* guiSystem_{nullptr};
            std::unordered_map<String, PanelEntry> panels_{};
    };
}
