#pragma once
#include <SDL3/SDL_video.h>
#include "Containers/String.h"


namespace BixEngine::Core
{
    class Window
    {
    public:
        // Crée une nouvelle fenêtre avec le titre et la taille spécifiés.
        // Le paramètre "resizable" contrôle si la fenêtre peut être redimensionnée.
        Window(const String& title, int width, int height, bool resizable = true);

        // Détruit la fenêtre et libère les ressources associées.
        ~Window();


        // Copie et déplacement interdits (une fenêtre ne peut pas être dupliquée).
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) noexcept = delete;
        Window& operator=(Window&&) noexcept = delete;

        // Retourne le pointeur SDL natif de la fenêtre.
        [[nodiscard]] SDL_Window* GetSDLWindow() const noexcept { return window_; }

        // Retourne la largeur actuelle de la fenêtre.
        [[nodiscard]] int GetWidth() const noexcept 
        { 
            int w, h;
            if (window_)
                SDL_GetWindowSize(window_, &w, &h);
            else
                w = width_;
            return w; 
        }

        // Retourne la hauteur actuelle de la fenêtre.
        [[nodiscard]] int GetHeight() const noexcept 
        { 
            int w, h;
            if (window_)
                SDL_GetWindowSize(window_, &w, &h);
            else
                h = height_;
            return h; 
        }

        // Indique si la fenêtre est valide.
        [[nodiscard]] bool IsValid() const noexcept { return window_ != nullptr; }

    private:
        SDL_Window* window_{nullptr}; // Pointeur SDL vers la fenêtre.
        int width_{}; // Largeur de la fenêtre.
        int height_{}; // Hauteur de la fenêtre.
    };
}
