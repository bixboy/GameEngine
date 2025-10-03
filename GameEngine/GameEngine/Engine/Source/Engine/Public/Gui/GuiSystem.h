#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL_events.h>

struct SDL_Window;
struct SDL_Renderer;

namespace Engine::Gui
{
    class GuiPanel;
    class LayoutSystem;

    class GuiSystem
    {
        public:
            GuiSystem();
            ~GuiSystem();

            GuiSystem(const GuiSystem&) = delete;
            GuiSystem& operator=(const GuiSystem&) = delete;
            GuiSystem(GuiSystem&&) noexcept = delete;
            GuiSystem& operator=(GuiSystem&&) noexcept = delete;

            bool Initialize(SDL_Window* window, SDL_Renderer* renderer);
            void Shutdown() noexcept;

            void BeginFrame();
            void EndFrame();
            void Render();

            void ProcessEvent(const SDL_Event& event);

            void RegisterPanel(GuiPanel& panel);
            void UnregisterPanel(GuiPanel& panel);

            [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }
            [[nodiscard]] bool WasIniSettingsLoaded() const noexcept { return iniSettingsLoaded_; }

            [[nodiscard]] LayoutSystem& GetLayoutSystem() noexcept { return *layoutSystem_; }
            [[nodiscard]] const LayoutSystem& GetLayoutSystem() const noexcept { return *layoutSystem_; }

        private:
            SDL_Window* window_{nullptr};
            SDL_Renderer* renderer_{nullptr};
            bool initialized_{false};
            bool frameBegun_{false};

            std::filesystem::path iniSettingsPath_{"imgui.ini"};
            std::string iniSettingsPathUtf8_{iniSettingsPath_.string()};
            bool iniSettingsLoaded_{false};

            std::vector<GuiPanel*> panels_{};
            std::unique_ptr<LayoutSystem> layoutSystem_{};
    };
}
