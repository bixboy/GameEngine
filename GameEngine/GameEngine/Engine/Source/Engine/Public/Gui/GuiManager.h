#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "Core/String.h"

namespace Engine::Gui
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

            void DrawAll();

            [[nodiscard]] std::vector<GuiPanel*> GetPanels();
            [[nodiscard]] std::vector<const GuiPanel*> GetPanels() const;

        private:
            GuiSystem* guiSystem_{nullptr};
            std::unordered_map<String, std::unique_ptr<GuiPanel>> panels_{};
    };
}
