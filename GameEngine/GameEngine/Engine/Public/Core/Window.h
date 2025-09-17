#pragma once
#include <SDL3/SDL.h>
#include <string>

class Window {
    
public:
    Window(const std::string& title, int width, int height, bool resizable = true);
    ~Window();

    SDL_Window* GetSDLWindow() const { return window; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

private:
    SDL_Window* window = nullptr;
    int width;
    int height;
};