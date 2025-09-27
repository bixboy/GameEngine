#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine::Gui
{
    class GuiSystem;
    class GuiPanel;

    class GuiManager
    {
        public:
            explicit GuiManager(GuiSystem& guiSystem);
            ~GuiManager();

            GuiPanel& CreatePanel(std::string name, std::string title);
            void RemovePanel(const std::string& name);

            [[nodiscard]] GuiPanel* FindPanel(const std::string& name) noexcept;
            [[nodiscard]] const GuiPanel* FindPanel(const std::string& name) const noexcept;

            void DrawAll();

            [[nodiscard]] std::vector<GuiPanel*> GetPanels();
            [[nodiscard]] std::vector<const GuiPanel*> GetPanels() const;

        private:
            GuiSystem* guiSystem_{nullptr};
            std::unordered_map<std::string, std::unique_ptr<GuiPanel>> panels_{};
    };
}
