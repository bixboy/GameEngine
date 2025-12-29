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

        
        [[nodiscard]] int GetWidth() const noexcept 
        { 
            int w, h;
            if (window_)
                SDL_GetWindowSize(window_, &w, &h);
            else
                w = width_;
            return w; 
        }

        
        [[nodiscard]] int GetHeight() const noexcept 
        { 
            int w, h;
            if (window_)
                SDL_GetWindowSize(window_, &w, &h);
            else
                h = height_;
            return h; 
        }

        
        [[nodiscard]] bool IsValid() const noexcept { return window_ != nullptr; }

    private:
        SDL_Window* window_{nullptr}; 
        int width_{}; 
        int height_{}; 
    };
}
