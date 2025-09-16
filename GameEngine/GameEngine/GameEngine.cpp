#define SDL_MAIN_HANDLED
#include "SDL3/SDL.h"
#include "iostream"

int main(int, char**) {
    
    // --- Initialisation SDL ---
    int count = SDL_GetNumVideoDrivers();
    std::cout << "Drivers vidéo disponibles: " << count << std::endl;
    for (int i = 0; i < count; i++) {
        std::cout << " - " << SDL_GetVideoDriver(i) << std::endl;
    }

    // --- Initialisation SDL ---
    // Tu peux forcer un driver ici si besoin (par ex. "windows" ou "dummy") :
    // SDL_setenv("SDL_VIDEODRIVER", "windows", 1);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL init error: " << SDL_GetError() << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return -1;
    }
    std::cout << "SDL Initialized!" << std::endl;
    std::cout << "Driver actif: " << (SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "none") << std::endl;

    // --- Création fenêtre ---
    SDL_Window* window = SDL_CreateWindow("Hello SDL3", 800, 600, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Window error: " << SDL_GetError() << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        SDL_Quit();
        return -1;
    }
    std::cout << "Window created!" << std::endl;

    // --- Création renderer ---
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "Renderer error: " << SDL_GetError() << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    std::cout << "Renderer created!" << std::endl;

    // --- Boucle principale ---
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Nettoyage écran
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Dessiner un carré rouge
        SDL_FRect rect{100.f, 100.f, 200.f, 200.f};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &rect);

        SDL_RenderPresent(renderer);
    }

    // --- Nettoyage ---
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Program terminated normally. Press Enter to exit..." << std::endl;
    std::cin.get();
    return 0;
}