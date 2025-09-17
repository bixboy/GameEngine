#include "../../Public/Core/Application.h"
#include "../../Public/Core/Window.h"
#include "../../Public/Input/Input.h"
#include "../../Public/Game/Entity.h"
#include "../../Public/Graphics/Renderer.h"
#include "../../Public/Core/Timer.h"
#include "iostream"
#include "SDL3/SDL.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

Application::Application() {}

Application::~Application() {
    Shutdown();
}

bool Application::Init() {
    
    SDL_SetAppMetadata("Example Custom engine", "1.0", "com.example.CustomEngine");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    window = std::make_unique<Window>("Custom Engine", WINDOW_WIDTH, WINDOW_HEIGHT);
    renderer = std::make_unique<Renderer>(window->GetSDLWindow());

    return true;
}

void Application::Run() {
    
    Input input;
    SDL_Event e;
    Timer timer;

    Entity player(300.f, 220.f, 50.f, 50.f, SDL_Color{255, 255, 255, 255});
    const float speed = 200.0f;

    while (running) {
        timer.Tick(); // calcule dt
        float dt = timer.GetDeltaTime();

        while (SDL_PollEvent(&e)) {
            input.ProcessEvent(e);
        }

        if (input.IsQuitRequested() || input.IsKeyDown(SDLK_ESCAPE)) {
            running = false;
        }

        if (input.IsKeyDown(SDLK_Z)) player.y -= speed * dt;
        if (input.IsKeyDown(SDLK_S)) player.y += speed * dt;
        if (input.IsKeyDown(SDLK_Q)) player.x -= speed * dt;
        if (input.IsKeyDown(SDLK_D)) player.x += speed * dt;

        renderer->SetColor(0, 0, 0, 255);
        renderer->Clear();

        player.Render(renderer->GetSDLRenderer());

        SDL_RenderDebugTextFormat(renderer->GetSDLRenderer(), 10, 10, "FPS: %.0f", timer.GetFPS());

        renderer->Present();
    }
}

void Application::Shutdown() {
    renderer.reset();
    window.reset();
    SDL_Quit();
}