#include "Core/Window.h"

namespace Engine::Core
{
    Window::Window(const std::string& title, int width, int height, bool resizable) : width_(width), height_(height)
    {
        const Uint32 flags = resizable ? SDL_WINDOW_RESIZABLE : 0;
        window_ = SDL_CreateWindow(title.c_str(), width, height, flags);
    }

    Window::~Window()
    {
        if (window_)
        {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
    }
}
