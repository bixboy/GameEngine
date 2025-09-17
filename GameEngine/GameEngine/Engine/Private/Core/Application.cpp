#include "../../Public/Core/Application.h"
#include "../../Public/Core/Window.h"
#include "../../Public/Input/Input.h"
#include "../../Public/Game/Entity.h"
#include "../../Public/Graphics/Renderer.h"
#include "../../Public/Core/Timer.h"
#include "../../Public/Core/SceneManager.h"
#include "../../Public/Game/EmptyScene.h"
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

void Application::Run()
{
    Input input;
    SDL_Event e;
    Timer timer;

    SceneManager sceneManager;
    sceneManager.SetScene(std::make_unique<EmptyScene>());

    while (running) {
        timer.Tick();
        float dt = timer.GetDeltaTime();

        // --- Events ---
        while (SDL_PollEvent(&e)) {
            input.ProcessEvent(e);
            if (sceneManager.GetScene()) {
                sceneManager.GetScene()->HandleEvent(e);
            }
        }

        if (input.IsQuitRequested() || input.IsKeyDown(SDLK_ESCAPE)) {
            running = false;
        }

        // --- Update ---
        if (sceneManager.GetScene()) {
            sceneManager.GetScene()->Update(dt);
        }

        // --- Render ---
        renderer->SetColor(0, 0, 0, 255);
        renderer->Clear();

        if (sceneManager.GetScene()) {
            sceneManager.GetScene()->Render(*renderer);
        }

        // HUD commun (FPS + nom de scène)
        SDL_RenderDebugTextFormat(
            renderer->GetSDLRenderer(),
            10, 10, "FPS: %.0f", timer.GetFPS()
        );

        if (sceneManager.GetScene()) {
            SDL_RenderDebugText(
                renderer->GetSDLRenderer(),
                10, 30, ("Scene: " + sceneManager.GetScene()->getName()).c_str()
            );
        }

        renderer->Present();
    }
}

void Application::Shutdown() {
    renderer.reset();
    window.reset();
    SDL_Quit();
}