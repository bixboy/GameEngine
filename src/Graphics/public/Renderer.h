#pragma once
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include "Math/Color.h"

namespace BixEngine::Graphics
{
    /**
     * Gère la création, destruction et configuration du moteur de rendu.
     */
    class Renderer
    {
    public:
        /**
         * @brief Crée un renderer associé à une fenêtre SDL.
         */
        explicit Renderer(SDL_Window* window, const char* driverName = nullptr, bool useVSync = false);

        ~Renderer();

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer(Renderer&&) noexcept = delete;
        Renderer& operator=(Renderer&&) noexcept = delete;

        static Renderer* Get() noexcept { return instance_; }

        [[nodiscard]] bool IsValid() const noexcept { return renderer_ != nullptr; }

        /** Efface le backbuffer avec une couleur donnée. */
        void Clear(Math::Color color) const noexcept;

        /** Affiche le contenu du backbuffer à l’écran. */
        void Present() const noexcept;

        /** Définit la couleur de dessin actuelle. */
        void SetColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) const noexcept;

        /** Met à jour le viewport lors d’un redimensionnement de fenêtre. */
        void UpdateViewport() const noexcept;

        /** Définit une résolution interne stable (facultatif). */
        void SetLogicalSize(int width, int height) const noexcept;

        [[nodiscard]] SDL_Renderer* GetSDLRenderer() const noexcept { return renderer_; }

    private:
        static inline Renderer* instance_ = nullptr;
        SDL_Renderer* renderer_{nullptr};
    };
}
