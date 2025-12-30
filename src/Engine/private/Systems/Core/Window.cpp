#include "Systems/Core/Window.h"
#include <SDL3/SDL.h>
#include "Debug/Logger.h"

namespace BixEngine::Core
{
    Window::Window(const String& title, int width, int height, bool resizable) : width_(width), height_(height)
    {
        SDL_WindowFlags flags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
        
        if (resizable)
            flags |= SDL_WINDOW_RESIZABLE;

        window_ = SDL_CreateWindow(title.c_str(), width, height, flags);

        if (!window_)
        {
            LOG_ERROR(String("Failed to create window: ") + SDL_GetError());
        }
    }

    Window::~Window()
    {
        if (window_)
        {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
    }

    int Window::GetWidth() const noexcept
    {
        if (!window_)
            return width_;
        
        int w = 0, h = 0;
        SDL_GetWindowSize(window_, &w, &h);
        return w;
    }

    int Window::GetHeight() const noexcept
    {
        if (!window_)
            return height_;
        
        int w = 0, h = 0;
        SDL_GetWindowSize(window_, &w, &h);
        return h;
    }

    int Window::GetPixelWidth() const noexcept
    {
        if (!window_)
            return width_;

        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(window_, &w, &h);
        return w;
    }

    int Window::GetPixelHeight() const noexcept
    {
        if (!window_)
            return height_;

        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(window_, &w, &h);
        return h;
    }

    float Window::GetDpiScale() const noexcept
    {
        const int logic = GetWidth();
        if (logic == 0)
            return 1.0f;
        
        return static_cast<float>(GetPixelWidth()) / static_cast<float>(logic);
    }
}