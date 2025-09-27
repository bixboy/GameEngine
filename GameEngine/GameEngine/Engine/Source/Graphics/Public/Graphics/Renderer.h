#pragma once
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include "Math/Color.h"

namespace Engine::Graphics
{
    class Renderer
    {
        public:
            explicit Renderer(SDL_Window* window);
            ~Renderer();

            Renderer(const Renderer&) = delete;
            Renderer& operator=(const Renderer&) = delete;
        
            Renderer(Renderer&&) noexcept = delete;
            Renderer& operator=(Renderer&&) noexcept = delete;

            [[nodiscard]] bool IsValid() const noexcept { return renderer_ != nullptr; }

            void Clear(Math::Color color) const;
            void Present() const;
            void SetColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) const;

            [[nodiscard]] SDL_Renderer* GetSDLRenderer() const noexcept { return renderer_; }

        private:
            SDL_Renderer* renderer_{nullptr};
    };
}
