#pragma once
#include <SDL3/SDL_video.h>
#include "Containers/String.h"

namespace BixEngine::Core
{
    class Window
    {
    public:
        Window(const String& title, int width, int height, bool resizable = true);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) noexcept = delete;
        Window& operator=(Window&&) noexcept = delete;

        [[nodiscard]] SDL_Window* GetSDLWindow() const noexcept { return window_; }

        // --- Taille Logique ---
        
        [[nodiscard]] int GetWidth() const noexcept;
        [[nodiscard]] int GetHeight() const noexcept;

        // --- Taille Réelle (Pixels) ---

        [[nodiscard]] int GetPixelWidth() const noexcept;
        [[nodiscard]] int GetPixelHeight() const noexcept;
        
        [[nodiscard]] float GetDpiScale() const noexcept;

        [[nodiscard]] bool IsValid() const noexcept { return window_ != nullptr; }

    private:
        SDL_Window* window_{nullptr};
        
        int width_{};
        int height_{};
    };
}