#include "../../Public/Core/Window.h"
#include "iostream"


Window::Window(const std::string& title, int width, int height, bool resizable) : width(width), height(height) 
{
    Uint32 flags = resizable ? SDL_WINDOW_RESIZABLE : 0;
    window = SDL_CreateWindow(title.c_str(), width, height, flags);
    if (!window) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
    }
}

Window::~Window() {
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}