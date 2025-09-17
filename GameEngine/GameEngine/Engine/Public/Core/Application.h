#pragma once
#define SDL_MAIN_HANDLED
#include "memory"
#include "SDL3/SDL.h"

class Renderer;
class Window;

class Application {
public:
    Application();
    ~Application();

    bool Init();
    void Run();
    void Shutdown();

private:
    std::unique_ptr<Window> window;
    std::unique_ptr<Renderer> renderer;
    bool running = true;
};
