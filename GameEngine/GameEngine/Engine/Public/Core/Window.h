#pragma once

#include <SDL3/SDL_video.h>

#include <string>

namespace Engine::Core {

class Window {
public:
    Window(const std::string& title, int width, int height, bool resizable = true);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept = delete;
    Window& operator=(Window&&) noexcept = delete;

    [[nodiscard]] SDL_Window* GetSDLWindow() const noexcept { return window_; }
    [[nodiscard]] int GetWidth() const noexcept { return width_; }
    [[nodiscard]] int GetHeight() const noexcept { return height_; }
    [[nodiscard]] bool IsValid() const noexcept { return window_ != nullptr; }

private:
    SDL_Window* window_{nullptr};
    int width_{};
    int height_{};
};

}  // namespace Engine::Core
