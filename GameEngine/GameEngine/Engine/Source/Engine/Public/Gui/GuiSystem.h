#pragma once

#include <vector>

#include <SDL3/SDL_events.h>

struct SDL_Window;
struct SDL_Renderer;

namespace Engine::Gui
{
    class GuiPanel;

    class GuiSystem
    {
        public:
            GuiSystem() = default;
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

        private:
            SDL_Window* window_{nullptr};
            SDL_Renderer* renderer_{nullptr};
            bool initialized_{false};
            bool frameBegun_{false};

            std::vector<GuiPanel*> panels_{};
    };
}
